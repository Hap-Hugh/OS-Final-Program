#ifndef _ZJUNIX_VFS_SEMAPHORE_H
#define _ZJUNIX_VFS_SEMAPHORE_H
#include <zjunix/vfs/vfs.h>
#include <zjunix/type.h>
#include <zjunix/lock.h>

typedef struct lock_t spinlock_t;
typedef enum key_t key_t;

enum key_t
{
	READ_AND_WRITE,
	READ_ONLY
};



struct semaphore {
	spinlock_t lock;
	u16 count;
	struct list_head wait_list;
};


/*These functions are defined in semaphore.c*/
void sema_init(struct semaphore *sem, int val);						//初始信号量的值，val

void down(struct semaphore *sem);									//获得信号量，特定情况要更改进程的状态TASK_UNINTERRUPTIBLE

int down_interruptible(struct semaphore *sem);						//休眠时需要将进程状态改变，可以被信号量中断的状态

int down_killable(struct semaphore *sem);

int down_trylock(struct semaphore *sem);

int down_timeout(struct semaphore *sem, long jiffies);

void up(struct semaphore *sem);

/*These functions are special for vfs about semaphore*/

int semget(key_t key, int num_sems, int sem_flags);


#endif 