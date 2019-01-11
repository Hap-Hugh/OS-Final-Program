#ifndef _VM_H
#define _VM_H

#include <zjunix/vm.h>


/*************VMA*************/
struct vma_struct* find_vma(struct mm_struct* mm, unsigned long addr);
unsigned long get_unmapped_area(unsigned long addr, unsigned long len, unsigned long flags);
struct vma_struct* find_vma_intersection(struct mm_struct* mm, unsigned long start_addr, unsigned long end_addr);
struct vma_struct* find_vma_and_prev(struct mm_struct* mm, unsigned long addr, struct vma_struct** prev);
struct vma_struct* find_vma_prepare(struct mm_struct* mm, unsigned long addr);
void insert_vma_struct(struct mm_struct* mm, struct vma_struct* area);
void exit_mmap(struct mm_struct* mm);
void pgd_delete(pgd_t* pgd);

#endif