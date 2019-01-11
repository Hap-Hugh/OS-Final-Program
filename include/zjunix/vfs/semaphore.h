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
void sema_init(struct semaphore *sem, int val);						//��ʼ�ź�����ֵ��val

void down(struct semaphore *sem);									//����ź������ض����Ҫ���Ľ��̵�״̬TASK_UNINTERRUPTIBLE

int down_interruptible(struct semaphore *sem);						//����ʱ��Ҫ������״̬�ı䣬���Ա��ź����жϵ�״̬

int down_killable(struct semaphore *sem);

int down_trylock(struct semaphore *sem);

int down_timeout(struct semaphore *sem, long jiffies);

void up(struct semaphore *sem);

/*These functions are special for vfs about semaphore*/

int semget(key_t key, int num_sems, int sem_flags);


#endif 