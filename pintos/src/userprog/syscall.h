#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include "lib/stdbool.h"
#include "lib/user/syscall.h"

void syscall_init (void);

void halt (void);

pid_t exec (const char *cmd_line);

void exit (int status);

int filesize (int fd);

int write (int fd, const void *buffer, unsigned size);

bool create (const char *file, unsigned initial_size);

int open (const char *file);

int filesize (int fd);

void seek (int fd, unsigned position);

void close (int fd);
#endif /* userprog/syscall.h */
