/* session/session.c */

#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/slab.h> 
#include <linux/mutex.h>
#include <linux/uaccess.h>
#include <linux/session.h>

static int is_init = 0;
static session *session_array[MAX_SESSIONS];
static DEFINE_MUTEX(global_vars);

static void init(void)
{
	int i;
	
	for (i = 0; i < MAX_SESSIONS; i++)
		session_array[i] = NULL;
}

static char* init_buffer(void)
{
	int i;
	char *buffer;
	
	buffer = kmalloc(BUFFER_SIZE, GFP_KERNEL);
	if (buffer == NULL)
		return NULL;
	
	for (i = 0; i < BUFFER_SIZE; i++)
		buffer[i] = 0;
		
	return buffer;
}

static int get_unused_sd(void)
{
	int i;
	
	for (i = 0; i < MAX_SESSIONS; i++) {
		if (session_array[i] == NULL)
			return i;
	}
	
	return -1;
}

static int create_session(int fd, struct file *f)
{
	int sd;
	ssize_t read;
	loff_t pos;
	session *s;
	char *buff;
	mm_segment_t old_fs;
	
	sd = get_unused_sd();
	if (sd == -1) 
		return -ENOMEM;
	
	s = kmalloc(sizeof(session), GFP_KERNEL);
	if (s == NULL)
		return -ENOMEM;
	
	buff = init_buffer();
	if (buff == NULL) {
		kfree(s);
		return -ENOMEM;
	}
	
	pos = 0;
	
	// load the file in our buffer
	if (!(f->f_flags & O_TRUNC)) {
		do {
			if (pos >= BUFFER_SIZE) {
				printk(KERN_ERR "[session] The file is too big to be read \n");
				goto out_error_dealloc;
			}

			old_fs = get_fs();
			set_fs(KERNEL_DS);
			read = vfs_read(f, buff + pos, sizeof(char), &pos);
			set_fs(old_fs);
			
			if (read < 0) {
				printk(KERN_ERR "[session] Error while reading file \n");
				goto out_error_dealloc;
			}
		} while (read > 0);
	}
	
	s->sd = sd;
	s->fd = fd;
	s->file = f;
	s->buffer = buff;
	s->position = (f->f_flags & O_APPEND) ? pos : 0;
	s->eof = pos;
	s->count = 1;
	mutex_init(&s->lock);
	
	session_array[sd] = s;
	
	return 0;
	
out_error_dealloc:
	kfree(buff);
	kfree(s);
	return -EFAULT;
}

static int remove_session(session *s)
{
	int written, error, ret;
	loff_t pos;
	struct file *f;
	struct dentry *dentry;
	mm_segment_t old_fs;
	
	mutex_lock(&s->lock);
	
	ret = 0;
	
	if (s->count > 1) {
		printk(KERN_INFO "[session] Session %d is shared, it will not be removed \n", s->sd);
		s->count--;
		goto out_unlock;
	}
	
	f = s->file;
	
	old_fs = get_fs();
	set_fs(KERNEL_DS);
	
	dentry = f->f_path.dentry;
	error = do_truncate(dentry, 0, ATTR_MTIME|ATTR_CTIME, f);
	if (error) {
		printk(KERN_ERR "[session] Error while truncating file before writing \n");
		ret = -EFAULT;
		goto out_unlock;
	}
	
	// flush the buffer on the file
	pos = 0;
	written = vfs_write(f, s->buffer, s->eof, &pos);
	set_fs(old_fs);
	
	if (written < s->eof) {
		printk(KERN_ERR "[session] Error while writing to file \n");
		ret = -EFAULT;
		goto out_unlock;
	}
	
	session_array[s->sd] = NULL;
	kfree(s->buffer);
	
	mutex_unlock(&s->lock);
	kfree(s);
	
	return 0;
	
out_unlock:
	mutex_unlock(&s->lock);
	return ret;
}

static session* get_session(struct file *f)
{
	int i;
	session *s;
	
	for (i = 0; i < MAX_SESSIONS; i++) {
		s = session_array[i];
		if (s != NULL) {
			if (s->file == f)
				return s;
		}
	}
	
	return NULL;
}

int open_session(int fd, struct file *f)
{
	int ret;
	session *s;
	
	mutex_lock(&global_vars);
	
	if (!is_init) {
		printk(KERN_INFO "[session] Data structures initialization \n");
		init();
		is_init = 1;
	}
	
	s = get_session(f);
	
	if (s != NULL) {
		printk(KERN_WARNING "[session] The session is already open \n");
		ret = -EINVAL;
		goto out_unlock;
	}
	
	ret = create_session(fd, f);
	if (ret < 0) {
		printk(KERN_ERR "[session] Error in creating session \n");
		goto out_unlock;
	}

out_unlock:
	mutex_unlock(&global_vars);
	return ret;
}

ssize_t read_session(struct file *f, char *buf, int count)
{
	session *s;
	int ret;
	
	mutex_lock(&global_vars);
	
	if (!is_init) {
		ret = -EINVAL;
		goto out_unlock_global;
	}
	
	if (count < 0) {
		ret = -EINVAL;
		goto out_unlock_global;
	}
	
	s = get_session(f);
	if (s == NULL) {
		ret = -EBADF;
		goto out_unlock_global;
	}
	
	mutex_lock(&s->lock);
	mutex_unlock(&global_vars);
	
	if (s->file == NULL) {
		ret = -ENOENT;
		goto out_unlock_session;
	}
	
	if (s->position + count >= BUFFER_SIZE)
		count = BUFFER_SIZE - s->position;
	
	if (copy_to_user(buf, s->buffer + s->position, count) != 0) {
		ret = -EFAULT;
		goto out_unlock_session;
	}
	
	ret = count;
	s->position += count;

out_unlock_session:
	mutex_unlock(&s->lock);
	return ret;

out_unlock_global:
	mutex_unlock(&global_vars);
	return ret;
}

ssize_t write_session(struct file *f, const char *buf, int count)
{
	session *s;
	int ret;
	
	mutex_lock(&global_vars);
	
	if (!is_init) {
		ret = -EINVAL;
		goto out_unlock_global;
	}
	
	if (count < 0) {
		ret = -EINVAL;
		goto out_unlock_global;
	}
	
	s = get_session(f);
	if (s == NULL) {
		ret = -EBADF;
		goto out_unlock_global;
	}
	
	mutex_lock(&s->lock);
	mutex_unlock(&global_vars);
	
	if (s->file == NULL) {
		ret = -ENOENT;
		goto out_unlock_session;
	}
	
	if (s->position + count > BUFFER_SIZE) {
		printk(KERN_ERR "[session] Error while writing, the file is becoming too big \n");
		ret = -EFAULT;
		goto out_unlock_session;
	}
	
	if (copy_from_user(s->buffer + s->position, buf, count) != 0) {
		ret = -EFAULT;
		goto out_unlock_session;
	}
	
	ret = count;
	s->position += count;
	
	if (s->position > s->eof)
		s->eof = s->position;
	
out_unlock_session:
	mutex_unlock(&s->lock);
	return ret;

out_unlock_global:
	mutex_unlock(&global_vars);
	return ret;
}

off_t lseek_session(struct file *f, off_t offset, int whence)
{
	int new_pos, ret;
	session *s;
	
	mutex_lock(&global_vars);
	
	if (!is_init) {
		ret = -EINVAL;
		goto out_unlock_global;
	}

	s = get_session(f);
	if (s == NULL) {
		ret = -EBADF;
		goto out_unlock_global;
	}
	
	mutex_lock(&s->lock);
	mutex_unlock(&global_vars);
	
	switch (whence) {
	case SEEK_SET:
		new_pos = offset;
		break;
	case SEEK_CUR:
		new_pos = s->position + offset;
		break;
	case SEEK_END:
		new_pos = s->eof + offset;
		break;
	default:
		printk(KERN_WARNING "[session] Undefined whence value \n");
		ret = -EINVAL;
		goto out_unlock_session;
	}
	
	if (new_pos < 0 || new_pos > BUFFER_SIZE) {
		ret = -EFAULT;
		goto out_unlock_session;
	} else {
		s->position = new_pos;
	}
	
	ret = s->position;
	if (s->position > s->eof)
		s->eof = s->position;
	
out_unlock_session:
	mutex_unlock(&s->lock);
	return ret;

out_unlock_global:
	mutex_unlock(&global_vars);
	return ret;
}

int share_session(struct file *f)
{
	session *s;
	
	mutex_lock(&global_vars);
	
	if (!is_init) {
		mutex_unlock(&global_vars);
		return -EINVAL;
	}

	s = get_session(f);
	if (s == NULL) {
		mutex_unlock(&global_vars);
		return -EBADF;
	}
	
	mutex_lock(&s->lock);
	mutex_unlock(&global_vars);
	
	// there is one process more sharing the session
	s->count++;
	
	mutex_unlock(&s->lock);
	
	return 0;
}

int close_session(struct file *f)
{
	int ret;
	session *s;
	
	mutex_lock(&global_vars);
	
	s = get_session(f);
	if (s == NULL) {
		printk(KERN_WARNING "[session] The session doesn't exist \n");
		return -EINVAL;
	}
	
	ret = remove_session(s);
	
	mutex_unlock(&global_vars);
	
	return ret;
}
