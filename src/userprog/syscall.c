#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/synch.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "threads/init.h"
#include "userprog/pagedir.h"

struct lock file_lock;

struct list fileList;

struct file_node {

  struct file* aFile;
  int fd;
  struct list_elem* node;

};

static void syscall_handler(struct intr_frame *);

void
syscall_init(void) {
  intr_register_int(0x30, 3, INTR_ON, syscall_handler, "syscall");
}
//TODO rewrite syscall handler to use an array of function pointers
//TODO implement exit() so that it unblocks processes that are waiting.
static void
syscall_handler(struct intr_frame *f UNUSED) {
  //printf("system call!\n");
  void *stack_pointer = f->esp;
  //printf("Address is '%p'\n", stack_pointer);
  //printf("PHYS_BASE is '%p'\n", PHYS_BASE);


  if (is_user_vaddr(stack_pointer)) {
    /* print a bunch of debug information*/
    /*
    printf("This is in user space\n");
    printf("Top of page is '%p'\n", pg_round_up(stack_pointer));
    printf("Bottom of page is '%p'\n", pg_round_down(stack_pointer));
    //struct thread *t = thread_current ();
    printf("value at stack pointer is '%i'\n", get_user(stack_pointer));
    for (int i = -20; i < 20; ++i) {
     printf("value at stack pointer+'%i' is '%i'\n", i, get_user(stack_pointer + i));
    }*/
    if(get_user(stack_pointer)==1){
      //power_off();
      thread_exit();

    }
    /*Right now every system call calls write and then kills the process*/
    int value=writesyscall(stack_pointer);
    //asm volatile ("movl %0, %%esp; jmp intr_exit" : : "g" (&f) : "memory");
    //NOT_REACHED();
    f->eax=value;
    return;
  }

  //
  //for(int i =pg_round_down(stack_pointer)+12; i<stack_pointer; ++i){

  //}
  printf("Syscall handler not implemented for this function!\n");
  thread_exit();
}

int writesyscall(void *sp) {
  int mode = get_user(sp + 4);
  int len = get_user(sp + 12);
  /*
  printf("len = %i\n", len);
  printf("mode = %i\n", mode);
  printf("buffer address1 = '%p'\n", get_user(sp + 8));
  printf("buffer address2 = '%p'\n", get_user(sp + 9));
  printf("buffer address3 = '%p'\n", get_user(sp + 10));
  printf("buffer address4 = '%p'\n", get_user(sp + 11));
  printf("bottom of page '%p'\n", pg_round_down(sp));
  */
  void *page_start = pg_round_down(sp);
  char *start_of_buffer = page_start + get_user(sp + 8);
  //printf("Buffer start address: '%p'\n", start_of_buffer);
  char buffer[2048];
  unsigned long buffer_address = get_user(sp + 11);
  buffer_address = buffer_address * 256 + get_user(sp + 10);
  buffer_address = buffer_address * 256 + get_user(sp + 9);
  buffer_address = buffer_address * 256 + get_user(sp + 8);
  //printf("Buffer address is '%p'\n", buffer_address);

  //hex_dump (page_start,buffer, 2048, true);
  //printf(get_user(start_of_buffer));
  if (mode == 1) {
    putbuf(buffer_address, len);
    return len;
  }

}


bool createsyscall(const char* file, unsigned initial_size)
{
  lock_acquire(&file_lock);
  bool success = filesys_create(file, initial_size);
  lock_release(&file_lock);
  return success;
}

int open(const char *file)
{
  lock_acquire(&file_lock);
  struct file *afile = filesys_open();

  if(!afile)
  {
    lock_release(&file_lock);
    return -1;
  }

  struct file_node* a_node = malloc(sizeof(struct file_node));

  a_node->aFile = afile;
  a_node->fd = thread_current()->fd;
  thread_current()->fd++;

  list_push_back(&fileList, a_node->node);

  lock_release(&file_lock);

  return a_node->fd;

}

int readsyscall(int fd, void* buffer, unsigned size)
{
  if(fd == 0)
  {
    int j = 0;
    uint8_t* locBuffer = (uint8_t*) buffer;
    for(j = 0; j < size; ++j)
    {
      locBuffer[j] = input_getc();
    }

    return size;
  }

  lock_acquire(&file_lock);

  //struct thread *t = thread_current();
  struct list_elem *e;

  struct file_node *F;

  e = list_begin(&fileList);

  while( F->fd != fd ||  e != list_end(&fileList) ) {

      F = list_entry(e, struct file_node, node);

    e = list_next(e);     
  }

  struct file *aFile = F->aFile;
  if(!aFile)
  {
    lock_release(&file_lock);
    return -1;
  }

  int num_bytes = file_read(aFile, buffer, size);
  lock_release(&file_lock);

  return num_bytes;

}
  
void seeksyscall(int fd, unsigned position)
{
  lock_acquire(&file_lock);

   struct list_elem *e;

  struct file_node *F;

  e = list_begin(&fileList);

  while( F->fd != fd ||  e != list_end(&fileList) ) {

      F = list_entry(e, struct file_node, node);

    e = list_next(e);     
  }

  struct file *aFile = F->aFile;
  if(!aFile)
  {
    lock_release(&file_lock);
    return -1;
  }

  file_seek(aFile, position);

  lock_release(&file_lock);

  
}