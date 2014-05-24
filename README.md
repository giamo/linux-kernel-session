Linux Kernel Session
=================================

Linux kernel extension that provides a variation of the default Linux file session. If a file is opened with this session system, all its content and changes made on it are flushed to main memory instead of the BUFFER CACHE, and all modifications are visible to other processes only when the file is closed.

Installation
------------

Copy the session/ folder within the kernel source code and compile it.

See the test client for how to use the new system calls once the kernel is in use.

System calls
------------

_int open\_session (int fd, struct file *f)_: opens the file in special session mode
_size\_t read\_session (struct file *f, char *buf, int count)_: reads _count_ bytes into _buf_ from the file session
_ssize\_t write\_session (struct file *f, const char *buf, int count)_: writes _count_ bytes from _buf_ into the file session
_off\_t lseek\_session (struct file *f, off\_t offset, int whence)_: moves the file pointer into the session
_int share\_session (struct file *f)_: adds the current process to the ones sharing the file session
_int close\_session (struct file *f)_: closes the file and flushes the session content on disk
