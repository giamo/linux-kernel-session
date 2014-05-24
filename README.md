Linux Kernel Session
=================================

Linux kernel extension that provides a variation of the default Linux file session. If a file is opened with this session system, all its content and changes made on it are flushed to main memory instead of the BUFFER CACHE, and all modifications are visible to other processes only when the file is closed.

Installation
------------

Copy the session/ folder within the kernel source code and compile it.

See the test client for how to use the new system calls once the kernel is in use.

System calls
------------

- `int open_session (int fd, struct file *f)`: opens the file in special session mode
- `size_t read_session (struct file *f, char *buf, int count)`: reads _count_ bytes into _buf_ from the file session
- `ssize_t write_session (struct file *f, const char *buf, int count)`: writes _count_ bytes from _buf_ into the file session
- `off_t lseek_session (struct file *f, off_t offset, int whence)`: moves the file pointer into the session
- `int share_session (struct file *f)`: adds the current process to the ones sharing the file session
- `int close_session (struct file *f)`: closes the file and flushes the session content on disk
