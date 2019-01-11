.globl  enter_new_pc

.set noreorder
.set noat
.align 2

#进入用户程序
#$a0: 用户程序入口函数 
#$a1: 用户栈指针
enter_new_pc:
    move   $t0, $a0
    move   $t1, $a1
    and    $a0, $zero
    and    $a1, $zero
    move   $sp, $t1
    jr     $t0
    nop

    