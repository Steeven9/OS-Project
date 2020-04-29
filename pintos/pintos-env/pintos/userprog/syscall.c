#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

#define READ(TYPE) *(TYPE *)(f->esp + read_bytes); read_bytes += sizeof(TYPE)

static void syscall_handler(struct intr_frame *);

void syscall_init(void) {
    intr_register_int(0x30, 3, INTR_ON, syscall_handler, "syscall");
}

/* Handle system calls, putting the return code in f->eax.
   Calls thread_unblock() on the parent when the child has 
   finished executing. */
static void syscall_handler(struct intr_frame *f) {
    unsigned int read_bytes = 0;
    enum syscall_nr num = READ(enum syscall_nr);

    switch (num) {
        int UNUSED fd;
        case SYS_WRITE:
            fd = READ(int);
            char *buf = (char *) READ(char *);
            unsigned int size = READ(unsigned int);
            putbuf(buf, size);
            break;
        case SYS_EXIT:
            f->eax = READ(int);
            *(thread_current()->parent_result) = f->eax;
            printf("%s: exit(%d)\n", thread_current()->name, f->eax);
            thread_unblock(thread_current()->parent);
            thread_exit();
            NOT_REACHED();
            break;
        default:
            printf("unknown system call!\n");
            thread_exit();
            NOT_REACHED();
    }
}
