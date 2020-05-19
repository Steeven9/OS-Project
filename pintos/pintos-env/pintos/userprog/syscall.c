#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "userprog/process.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "devices/shutdown.h"
#include "threads/synch.h"
#include "lib/kernel/hash.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "threads/malloc.h"

#define READ(TYPE) *(TYPE *)(f->esp + read_bytes); read_bytes += sizeof(TYPE)
#define CHECK_POINTER(PTR) if (PTR == NULL || !is_user_vaddr(PTR) || lookup_page(active_pd(), PTR, false) == NULL) { \
f->eax = ERROR_EXIT; \
goto EXIT; \
}
#define ERROR_EXIT -1

unsigned item_hash (const struct hash_elem * e, void * aux UNUSED);
bool item_compare(const struct hash_elem * a, const struct hash_elem * b, void * aux UNUSED);

//Hash table item structure (map fd to file struct)
struct item {
    int fd;
    struct file * file;
    struct hash_elem elem;
};

//Hashing function for hash table
unsigned item_hash (const struct hash_elem * e, void * aux UNUSED) {
    struct item * i = hash_entry(e, struct item, elem);
    return hash_int(i->fd);
}

//Compare function for hash table
bool item_compare(const struct hash_elem * a, const struct hash_elem * b, void * aux UNUSED) {
    struct item * i_a = hash_entry(a, struct item, elem);
    struct item * i_b = hash_entry(b, struct item, elem);
    return i_a->fd < i_b->fd;
}

static void syscall_handler(struct intr_frame *);
static struct semaphore filesys_sem;
static struct hash fd_table;

void syscall_init(void) {
    intr_register_int(0x30, 3, INTR_ON, syscall_handler, "syscall");
    sema_init(&filesys_sem, 1);
    hash_init(&fd_table, item_hash, item_compare, NULL);
}

/* Handle system calls, putting the return code in f->eax.
   Calls thread_unblock() on the parent when the child has 
   finished executing. */
static void syscall_handler(struct intr_frame *f) {
    unsigned int read_bytes = 0;
    enum syscall_nr num = READ(enum syscall_nr);

    switch (num) {
        struct item item;
        struct hash_elem * e;
        case SYS_WRITE:
            sema_down(&filesys_sem);
            int fd = READ(int);
            char *buf = (char *) READ(char *);
            CHECK_POINTER(buf);
            if (buf == NULL || !is_user_vaddr(buf) || lookup_page(active_pd(), buf, false) == NULL) {
                f->eax = ERROR_EXIT;
                goto EXIT;
            }
            unsigned int size = READ(unsigned int);

            if (fd == 0) {
                //Stdin fd (not good)
                f->eax = ERROR_EXIT;
                goto EXIT;
            } else if (fd == 1) {
                //Output to stdout
                putbuf(buf, size);
            } else {
                item.fd = fd;
                e = hash_find(&fd_table, &item.elem);
                f->eax = file_write(hash_entry(e, struct item, elem)->file, buf, size);
            }

            sema_up(&filesys_sem);
            break;
        case SYS_EXIT:
            f->eax = READ(int);
        EXIT:
            thread_current()->result_code = f->eax;
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
            CHECK_POINTER(cmdline);
            f->eax = process_execute(cmdline);
            break;
        case SYS_HALT:
            shutdown_power_off();
            NOT_REACHED();
        char * name;
        case SYS_CREATE:
            sema_down(&filesys_sem);
            name = (char *) READ(char *);
            CHECK_POINTER(name);
            unsigned initial_size = READ(unsigned);
            f->eax = filesys_create(name, initial_size);
            sema_up(&filesys_sem);
            break;
        case SYS_REMOVE:
            sema_down(&filesys_sem);
            name = (char *) READ(char *);
            CHECK_POINTER(name);
            f->eax = filesys_remove(name);
            sema_up(&filesys_sem);
            break;
        case SYS_OPEN:
            sema_down(&filesys_sem);
            name = (char *) READ(char *);
            CHECK_POINTER(name);
            struct file * file = filesys_open(name);
            static unsigned next_fd = 2;    //0 and 1 are reserved to stdin and out
            struct item * new_item = malloc(sizeof(struct item));
            new_item->fd = next_fd++;
            new_item->file = file;
            hash_insert(&fd_table, &new_item->elem);
            f->eax = new_item->fd;
            //Memory leaks weee
            sema_up(&filesys_sem);
            break;
        case SYS_CLOSE:
            sema_down(&filesys_sem);
            fd = READ(int);
            if (fd == 0 || fd == 1) {
                //Stdin/out fd (not good)
                f->eax = ERROR_EXIT;
                goto EXIT;
            }
            item.fd = fd;
            e = hash_find(&fd_table, &item.elem);
            file_close(hash_entry(e, struct item, elem)->file);
            sema_up(&filesys_sem);
            break;
        case SYS_READ:
            sema_down(&filesys_sem);
            fd = READ(int);
            buf = READ(void *);
            CHECK_POINTER(buf);
            size = READ(unsigned);
            if (fd == 0) {
                //Read from stdin
                PANIC("not implemented");
            } else if (fd == 1) {
                //Stdout fd (not good)
                f->eax = ERROR_EXIT;
                goto EXIT;
            } else {
                item.fd = fd;
                e = hash_find(&fd_table, &item.elem);
                f->eax = file_read(hash_entry(e, struct item, elem)->file, buf, size);
            }
            sema_up(&filesys_sem);
            break;
        case SYS_SEEK:
            sema_down(&filesys_sem);
            fd = READ(int);
            unsigned position = READ(unsigned);
            if (fd == 0 || fd == 1) {
                //Stdin/out fd (not good)
                f->eax = ERROR_EXIT;
                goto EXIT;
            }
            item.fd = fd;
            e = hash_find(&fd_table, &item.elem);
            file_seek(hash_entry(e, struct item, elem)->file, position);
            sema_up(&filesys_sem);
            break;
        case SYS_TELL:
            sema_down(&filesys_sem);
            fd = READ(int);
            if (fd == 0 || fd == 1) {
                //Stdin/out fd (not good)
                f->eax = ERROR_EXIT;
                goto EXIT;
            }
            item.fd = fd;
            e = hash_find(&fd_table, &item.elem);
            f->eax = (int)file_tell(hash_entry(e, struct item, elem)->file);
            sema_up(&filesys_sem);
            break;
        case SYS_FILESIZE:
            sema_down(&filesys_sem);
            fd = READ(int);
            if (fd == 0 || fd == 1) {
                //Stdin/out fd (not good)
                f->eax = ERROR_EXIT;
                goto EXIT;
            }
            item.fd = fd;
            e = hash_find(&fd_table, &item.elem);
            f->eax = (int)file_length(hash_entry(e, struct item, elem)->file);
            sema_up(&filesys_sem);
            break;
        default:
            PANIC("unknown system call");
            thread_exit();
            NOT_REACHED();
    }
}
