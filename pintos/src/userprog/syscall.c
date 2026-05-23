#include "userprog/syscall.h"
#include <stdio.h>
#include "lib/kernel/stdio.h"
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "devices/shutdown.h"

static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  switch (*(int*)f->esp)
  {
  case SYS_EXIT:
    exit(*((int*)f->esp + 1));
    break;
  
  default:
    break;
  }

  //printf ("system call!\n");
  //thread_exit ();
}

void halt (void)
{
  shutdown_power_off(); 
}

void exit (int status)
{
  thread_current()->status = status;
  thread_exit();
}

int write (int fd, const void *buffer, unsigned size)
{
 if(fd == 1)
 {
  char* buff = buffer;
  putbuf(buff, size);

  return size;
 }  
}