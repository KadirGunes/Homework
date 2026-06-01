#include "userprog/syscall.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lib/kernel/stdio.h"
#include <syscall-nr.h>

#define USERPROG
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"

#include "devices/shutdown.h"
#include "devices/input.h"

#include "filesys/filesys.h"
#include "filesys/file.h"

#include "userprog/process.h"

static struct lock file_lock;

static void syscall_handler (struct intr_frame *);

void is_ptr_valid(const void* ptr);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  is_ptr_valid((int*)f->esp);

  switch (*(int*)f->esp)
  {
  case SYS_WRITE:
  {
    is_ptr_valid((int*)f->esp + 1);
    is_ptr_valid((void*)(*((int*)f->esp + 2)));
    is_ptr_valid((unsigned*)f->esp + 3);

    int fd = *((int*)f->esp + 1);
    void* buffer = (void*)(*((int*)f->esp + 2));
    unsigned size = *((unsigned*)f->esp + 3);

    f->eax = write(fd, buffer, size);
    break;
  }
  case SYS_WAIT:
    f->eax = process_wait(*((tid_t*)f->esp + 1));
    break;
  case SYS_EXIT:
    is_ptr_valid((int*)f->esp + 1);
    
    exit(*((int*)f->esp + 1));
    break;
  case SYS_CREATE:
  {
    is_ptr_valid((const char *)(*((int*)f->esp + 1)));

    const char* file = (const char *)(*((int*)f->esp + 1));
    unsigned int* size = ((unsigned int*)f->esp + 2);

    if(file == NULL) exit(-1);

    file_acquire_lock();
    f->eax = create(file, *size);
    file_realese_lock();
    break;
  }
  case SYS_OPEN:
  {
    is_ptr_valid((const char *)(*((int*)f->esp + 1)));

    file_acquire_lock();
    f->eax = open((const char *)(*((int*)f->esp + 1)));
    file_realese_lock();
    break;
  }
  case SYS_EXEC:
    is_ptr_valid((const char *)(*((int*)f->esp + 1)));

    f->eax = exec((const char *)(*((int*)f->esp + 1)));
    break;
  case SYS_CLOSE:
    file_acquire_lock();
    close(*((int*)f->esp + 1));
    file_realese_lock();
    break;
  case SYS_REMOVE:
    file_acquire_lock();
    f->eax = remove((const char *)(*((int*)f->esp + 1)));
    file_realese_lock();
    break;
  case SYS_READ:
    is_ptr_valid((int*)f->esp + 1);
    is_ptr_valid((void*)(*((int*)f->esp + 2)));
    is_ptr_valid((unsigned*)f->esp + 3);

    int fd = *((int*)f->esp + 1);
    void* buffer = (void*)(*((int*)f->esp + 2));
    unsigned size = *((unsigned*)f->esp + 3);  

    char* buf = (char*)buffer;
    for (unsigned i = 0; i < size; i++)
        is_ptr_valid(buf + i);

    f->eax = read(fd, buffer, size);
    break;
  case SYS_FILESIZE:
    file_acquire_lock();
    f->eax = filesize(*((int*)f->esp + 1));
    file_realese_lock();
    break;
  case SYS_SEEK:
    file_acquire_lock();
    seek(*((int*)f->esp + 1), *((unsigned*)f->esp + 2));
    file_realese_lock();
    break;
  case SYS_TELL:
    file_acquire_lock();
    f->eax = tell(*((int*)f->esp + 1));
    file_realese_lock();
    break;
  case SYS_HALT:
    halt();
    break;
  default:
    printf ("system call! %d\n", *(int*)f->esp);
    thread_exit ();
    break;
  }
}

void halt (void)
{
  shutdown_power_off(); 
}

pid_t exec (const char *cmd_line)
{
  file_acquire_lock();
  char* temp = (char*)malloc(strlen(cmd_line) + 1);

  memcpy(temp, cmd_line, strlen(cmd_line) + 1);

  char* save_pointer;
  temp = strtok_r(temp, " ", &save_pointer);

  struct file* f = filesys_open(temp); 

  if(f == NULL)
  {
    file_realese_lock();
    return -1;
  }

  file_close(f);
  file_realese_lock();

  return process_execute(cmd_line);
}

void exit (int status)
{
  if(thread_current()->parent == NULL)
  {
    thread_current()->exit_status = status;
    thread_exit();
  }

  enum intr_level old_level = intr_disable(); 

  struct list_elem* e_p = list_begin(&thread_current()->parent->processes);
  struct proc* f;

  while (e_p != list_end(&thread_current()->parent->processes))
  {
    f = list_entry(e_p, struct proc, elem);
    
    if(f->tid == thread_current()->tid)
    {
      //f->used = true;
      f->exit_status = status;
      break;
    }

    e_p = list_next(e_p);
  }

  intr_set_level(old_level);
  
  thread_current()->exit_status = status;
  //thread_sema_up(thread_current()->tid);
  sema_up(&f->wait_sema);
  thread_exit();
}

int write (int fd, const void *buffer, unsigned size)
{
  if(fd == 1)
  {
   putbuf(buffer, size);
   return size;
  }
  else if (fd <= 0) return 0;

  struct thread* cur = thread_current();
  
  //if(cur->parent != NULL && cur->files[fd - 2] == cur->parent->executable) return 0;

  if(fd >= cur->next_fd) return 0;

  file_acquire_lock();
  int _return = file_write(cur->files[fd - 2], buffer, size);
  file_realese_lock();

  return _return;
}

bool create (const char *file, unsigned initial_size)
{
  //if(file == NULL || initial_size == 0) return false;
  
  return filesys_create(file, initial_size);
}

int open (const char *file)
{
  if(file == NULL)
    return -1;

  struct file* fptr = filesys_open(file);
  
  if(fptr == NULL)
    return -1;

  struct thread* cur = thread_current();
  
  cur->files[cur->next_fd - 2] = fptr;
  cur->next_fd++;

  return (cur->next_fd - 1);
}

void seek (int fd, unsigned position)
{
  struct file* f = thread_current()->files[fd - 2];

  file_seek(f, position);
}

unsigned tell (int fd)
{
  return file_tell(thread_current()->files[fd - 2]);
}

int filesize (int fd)
{
  struct file* f = thread_current()->files[fd - 2];

  return file_length(f);
}

void close (int fd)
{
  struct thread* t = thread_current();

  if(fd >= t->next_fd || t->files[fd - 2] == NULL) return;

 file_close(t->files[fd - 2]); 

 t->files[fd - 2] = NULL;
}

bool remove (const char *file)
{
  return filesys_remove(file);
}

int read (int fd, void *buffer, unsigned size)
{
  if(fd == 0)
  {
    uint8_t* real_buffer = (uint8_t*)buffer;

    for (int i = 0; i < size; i++)
    {
      real_buffer[i] = input_getc();
    }
    
    return size;
  }
  else if (fd == 1 || fd < 0)
  {
    return -1;
  }

  struct thread* cur = thread_current();

  if(fd >= cur->next_fd) return -1;

  file_acquire_lock();
  int _return = file_read(cur->files[fd - 2], buffer, size);
  file_realese_lock();

  return _return; 
}

void is_ptr_valid(const void* ptr)
{
  if(ptr == NULL || !is_user_vaddr(ptr) || pagedir_get_page(thread_current()->pagedir, ptr) == NULL) 
    return exit(-1);
}