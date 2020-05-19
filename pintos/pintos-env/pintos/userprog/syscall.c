#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "userprog/process.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"

#define READ(TYPE) *(TYPE *)(f->esp + read_bytes); read_bytes += sizeof(TYPE)
#define ERROR_EXIT -1

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
            if (buf == NULL || !is_user_vaddr(buf) || lookup_page(active_pd(), buf, false) == NULL) {
                f->eax = ERROR_EXIT;
                goto EXIT;
            }
            unsigned int size = READ(unsigned int);
            putbuf(buf, size);
            break;
        case SYS_EXIT:
            f->eax = READ(int);
        EXIT:
            thread_current()->result_code = f->eax;
            printf("%s: exit(%d)\n", thread_current()->name, f->eax);
            if (thread_current()->parent) {
                thread_unblock(thread_current()->parent);
            }
            thread_exit();
            NOT_REACHED();
            break;
        tid_t child;
        case SYS_WAIT:
            child = READ(tid_t);
            f->eax = process_wait(child);
            break;
        char *cmdline;
        case SYS_EXEC:
            cmdline = READ(char *);
            if (cmdline == NULL || !is_user_vaddr(cmdline) || lookup_page(active_pd(), cmdline, false) == NULL) {
                f->eax = ERROR_EXIT;
                goto EXIT;
            }
            f->eax = process_execute(cmdline);
            break;
        default:
            PANIC("unknown system call");
            thread_exit();
            NOT_REACHED();
    }
}
