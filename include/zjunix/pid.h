#ifndef  _ZJUNIX_PID_H
#define  _ZJUNIX_PID_H

#define  PID_NUM  256  //最大进程数
#define  PID_BYTES ((PID_NUM + 7) >> 3)
#define  IDLE_PID  0            //idle 进程
#define  INIT_PID  1            //for kernel shell
#define  PIDMAP_INIT  0x01
#define  PID_MIN   1

typedef unsigned int pid_t;

unsigned char pidmap[PID_BYTES];    //pid 位图
pid_t next_pid;      //pid cache

void init_pid();
int pid_alloc(pid_t *ret);
int pid_free(pid_t num);
int pid_check(pid_t pid);

#endif