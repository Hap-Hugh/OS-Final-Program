.globl  switch_ex
.globl  switch_wa

.set noreorder
.set noat
.align 2

#用于进程退出是加载执行新的进程
switch_ex:
    move  $sp, $a0
	lw $a2, 0($sp) # EPC
	lw $t3, 104($sp) # HI
	lw $t4, 108($sp) # LO
	mtc0 $a2, $14
	mthi $t3
	mtlo $t4
	lw $at, 4($sp)
	lw $v0, 8($sp)
	lw $v1, 12($sp)
	lw $a0, 16($sp)
	lw $a1, 20($sp)
	lw $a2, 24($sp)
	lw $a3, 28($sp)
	lw $t0, 32($sp)
	lw $t1, 36($sp)
	lw $t2, 40($sp)
	lw $t3, 44($sp)
	lw $t4, 48($sp)
	lw $t5, 52($sp)
	lw $t6, 56($sp)
	lw $t7, 60($sp)
	lw $s0, 64($sp)
	lw $s1, 68($sp)
	lw $s2, 72($sp)
	lw $s3, 76($sp)
	lw $s4, 80($sp)
	lw $s5, 84($sp)
	lw $s6, 88($sp)
	lw $s7, 92($sp)
	lw $t8, 96($sp)
	lw $t9, 100($sp)
	lw $gp, 112($sp)
	lw $k1, 116($sp)  #sp
	lw $fp, 120($sp)
	lw $ra, 124($sp)	
	move $sp, $k1
	mtc0 $zero, $9	#count = 0
	eret


#把ra的值存入epc中
switch_wa:
	move	$k1, $sp
	move    $sp, $a1


save_context:
	sw $at, 4($sp)
	sw $v0, 8($sp)
	sw $v1, 12($sp)
	sw $a0, 16($sp)
	sw $a1, 20($sp)
	sw $a2, 24($sp)
	sw $a3, 28($sp)
	sw $t0, 32($sp)
	sw $t1, 36($sp)
	sw $t2, 40($sp)
	sw $t3, 44($sp)
	sw $t4, 48($sp)
	sw $t5, 52($sp)
	sw $t6, 56($sp)
	sw $t7, 60($sp)
	sw $s0, 64($sp)
	sw $s1, 68($sp)
	sw $s2, 72($sp)
	sw $s3, 76($sp)
	sw $s4, 80($sp)
	sw $s5, 84($sp)
	sw $s6, 88($sp)
	sw $s7, 92($sp)
	sw $t8, 96($sp)
	sw $t9, 100($sp)
	sw $gp, 112($sp)
	sw $k1, 116($sp)
	sw $fp, 120($sp)
	sw $ra, 124($sp)
	mfhi $t3
	mflo $t4
	sw $ra, 0($sp) # EPC !!!!

	sw $t3, 104($sp) # HI
	sw $t4, 108($sp) # LO

	j  switch_ex

			