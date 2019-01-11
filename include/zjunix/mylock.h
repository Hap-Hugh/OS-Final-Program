#ifndef _ZJUNIX_LOCK_H
#define _ZJUNIX_LOCK_H

#define MAX_CON 8
#define CMP_PAIR(x1,y1,x2,y2) ((x1)<(x2)||((x1)==(x2)&&(y1)<(y2)))

struct lock_t {
    int hashMap[MAX_CON], number[MAX_CON];
    char choosing[MAX_CON];
};

extern void init_lock(struct lock_t *lock);
extern void lockup(struct lock_t *lock);
extern void unlock(struct lock_t *lock);

void do_lock(struct lock_t *lock, int pid);
void do_unlock(struct lock_t *lock, int pid);

#endif  // !_ZJUNIX_LOCK_H