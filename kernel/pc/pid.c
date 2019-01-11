#include <zjunix/pid.h>
#include <driver/vga.h>

//初始化pid位图
void init_pid() {
    int i;
    for (i = 1; i < PID_BYTES; i++) 
        pidmap[i] = 0;

    pidmap[0] = PIDMAP_INIT; //为idle进程分配pid
    next_pid = PID_MIN;  //设置pid查找的cache，加速pid分配过程
}

//检查pid是否已被分配
//已被分配则返回1，否则返回0
int pid_check(pid_t pid) {
    int index, off;

    //pid 大于允许的最大pid号
    if (pid >= PID_NUM) {
        return 0;
    }
   
    //查找相应的pid位图是否为1，如果为1则表示已分配
    index = pid >> 3;
    off = pid & 0x07;
    if (pidmap[index] & (1 << off))
        return 1;
    else
        return 0;
}

//分配pid
//分配成功则返回0，否则返回1
//ret存放新分配的pid号
int pid_alloc(pid_t *ret) {
    int i, index, off;
    int val;
    pid_t tmp = next_pid; 

    //从全局变量next_pid开始遍历查找pid位图,寻找空闲pid号
    for (i = 0; i < PID_NUM; i++) {
        index = tmp >> 3;
        off = tmp & 0x07;
        val = pidmap[index] >> off;

        if (!(val & 0x01)) 
            break;
        else
            tmp = (tmp + 1) % PID_NUM;
    }

    //未找到
    if (i == PID_NUM)
        return 1;
    
    //找到，分配之
    pidmap[index] |= 1 << off; 
    *ret = tmp;
    next_pid = *ret + 1;
    return 0;
}

//释放数字为pid的进程pid号
//是否成功则返回0，否则返回1
int pid_free(pid_t pid) {
    int res;
    int index, off;

    //首先检查pid是否已被分配
    res = pid_check(pid);
    if (res == 0)
        return 1;
    else {
        //检查通过，释放之
        index = pid >> 3;
        off = pid & 0x07;
        pidmap[index] &= (~(1 << off));
        return 0;
    }
}
