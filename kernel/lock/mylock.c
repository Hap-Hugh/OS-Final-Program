#include "mylock.h"
#include <intr.h>
#include <zjunix/pc.h>

extern int init_done;

void init_lock(struct lock_t *lock) {
    int i;
    for (i=0; i<MAX_CON; i++) {
        lock->hashMap[i] = -1;
        lock->choosing[i] = 0;
        lock->number[i] = 0;
    }
}

void lockup(struct lock_t *lock) {
    #ifdef LOCK_DEBUG
        kernel_printf("lockup: %d\n", init_done);
    #endif
       
    if (init_done == 1) {
        do_lock(lock, current_task->ASID);
    }
        
}

void do_lock(struct lock_t *lock, int pid) {
    unsigned int old_ie, i;
    int j, temp = 0;

entry:
    old_ie = disable_interrupts();
    // Hash
    i = pid % MAX_CON;
#ifdef LOCK_DEBUG
    kernel_printf("PID:%d  i:%d\n", pid, i);
#endif  // LOCK_DEBUG

    if (lock->hashMap[i] >= 0) {        
        for (j=0; j<MAX_CON; j++) {
            if (lock->hashMap[j] < 0) {
                i = j;
                break;
            }
        }
        if (j == MAX_CON) {
#ifdef LOCK_DEBUG
            kernel_printf("No empty place\n");
#endif  // LOCK_DEBUG
            if (old_ie) enable_interrupts();
            for (j=0; j<100000; j++)
                ;       // sleep
            goto entry;
        }
    }
    lock->hashMap[i] = pid;
#ifdef LOCK_DEBUG
    kernel_printf("Hash succeed. PID:%d  i:%d\n", pid, i);
#endif  // LOCK_DEBUG
    if (old_ie) {
        enable_interrupts();
    } 

    lock->choosing[i] = 1;
    for (j=0; j<MAX_CON; j++) {
        if (lock->number[j] > temp) temp = lock->number[j];
    }
    lock->number[i] = temp+1;
    lock->choosing[i] = 0;

    for (j=0; j<MAX_CON; j++) {
        while (lock->choosing[j]) {

        }

        while (lock->number[j]!=0 && 
        CMP_PAIR(lock->number[j],j,lock->number[i],i)) {

        }
    }
}

void unlock(struct lock_t *lock) {
    #ifdef LOCK_DEBUG
    kernel_printf("unlock: %d\n", init_done);
    #endif
    if (init_done == 1)
        do_unlock(lock, current_task->ASID);
}

void do_unlock(struct lock_t *lock, int pid)
{
    int i, j;
    for (i=0; i<MAX_CON; i++) {
        if (lock->hashMap[i] == pid) break;
    }
    if (i==MAX_CON) return;
    lock->number[i] = 0;
    lock->hashMap[i] = 0;
}
