Linux Kernel Session
=================================

Linux kernel extension that provides a variation of the default Linux file session. If a file is opened with this session system, all its content and changes made on it are flushed to main memory instead of the BUFFER CACHE, and all modifications are visible to other processes only when the file is closed.

Installation
------------

Copy the session/ folder within the kernel source code and compile it.

See the test client for how to use the new system calls once the kernel is in use.
