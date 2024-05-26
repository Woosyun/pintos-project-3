#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

void syscall_init (void);

void sys_exit (int);

bool sys_munmap (int mmap_id);

#endif /* userprog/syscall.h */
