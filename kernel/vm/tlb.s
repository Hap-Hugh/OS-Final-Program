.globl  set_tlb_asid

.set noreorder
.set noat
.align 2

set_tlb_asid:
    mtc0    $zero, $5   #PageMask
    mfc0    $t0, $10    #entry_hi
   # lui     $t1, 0xffff
   # ori     $t1, $t1, 0xe000
    li      $t1, 0xffffe000
    and     $t0, $t0, $t1
    or      $t0, $t0, $a0
    mtc0    $t0, $10
    nop
    nop
    jr      $ra


