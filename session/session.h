/* include/linux/session.h */

#ifndef __SESSION_H__
#define __SESSION_H__

#include <linux/file.h>
#include <linux/fcntl.h>

#define BUFFER_SIZE (16 * 1024)
#define MAX_SESSIONS 128

typedef struct session {
	int sd;
	int fd;
	struct file *file;
	char *buffer;
	loff_t position;
	loff_t eof;
	int count;
	struct mutex lock;
} session;

extern int open_session (int fd, struct file *f);
extern ssize_t read_session (struct file *f, char *buf, int count);
extern ssize_t write_session (struct file *f, const char *buf, int count);
extern off_t lseek_session (struct file *f, off_t offset, int whence);
extern int share_session (struct file *f);
extern int close_session (struct file *f);

#endif
