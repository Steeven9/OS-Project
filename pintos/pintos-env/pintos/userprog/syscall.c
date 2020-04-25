#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

#define POP_old(TYPE) *(TYPE *)f->esp; f->esp += sizeof(TYPE)
#define POP(TYPE) *(TYPE *)(f->esp + read_bytes); read_bytes += sizeof(TYPE)

static void syscall_handler(struct intr_frame *);

void syscall_init(void) {
    intr_register_int(0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void syscall_handler(struct intr_frame *f) {
    // set f->eax to return value
    // putbuf
    unsigned int read_bytes = 0;
    enum syscall_nr num = POP(enum syscall_nr);
    switch (num) {
        int UNUSED fd;
        case SYS_WRITE:
            fd = POP(int);
            char *buf = (char *) POP(char *);
            unsigned int size = POP(unsigned int);
            //printf("write %d bytes :%s:\n", size, buf);
            putbuf(buf, size);
            break;
        case SYS_EXIT:
            printf("exiting\n");
            f->eax = POP(int);
            thread_exit();
            NOT_REACHED();
            break;
        default:
            printf("unknown system call!\n");
            thread_exit();
    }

    // thread_exit();
}
