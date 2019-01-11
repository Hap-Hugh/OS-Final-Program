#include "vm.h"
#include <zjunix/slab.h>
#include <zjunix/utils.h>
#include <zjunix/pc.h>
#include <driver/vga.h>
#include <arch.h>




/************mm_struct************/
struct mm_struct* mm_create()
{
    struct mm_struct* mm;

    mm = kmalloc(sizeof(*mm));
#ifdef VMA_DEBUG
    kernel_printf("mm_create: %x\n", mm);
#endif //VMA_DEBUG
    if (mm) {
        kernel_memset(mm, 0, sizeof(*mm));
        mm->pgd = kmalloc(PAGE_SIZE); 
        if (mm->pgd) {
            kernel_memset(mm->pgd, 0, PAGE_SIZE);
            return mm;
        }
#ifdef VMA_DEBUG
        kernel_printf("mm_create fail\n");
#endif //VMA_DEBUG
        kfree(mm);
    }
    return 0;
}

void mm_delete(struct mm_struct* mm)
{
#ifdef VMA_DEBUG
    kernel_printf("mm_delete: pgd %x\n", mm->pgd);
#endif //VMA_DEBUG
    pgd_delete(mm->pgd);
    exit_mmap(mm);
#ifdef VMA_DEBUG
    kernel_printf("exit_mmap done\n");
#endif //VMA_DEBUG
    kfree(mm);
}

void pgd_delete(pgd_t* pgd)
{
    int i,j;
    unsigned int pde, pte;
    unsigned int* pde_ptr;

    for (i = 0; i < 1024; i++) {
        pde = pgd[i];
        pde &= PAGE_MASK;
       
        if (pde == 0)  //不存在二级页表
            continue;
#ifdef VMA_DEBUG
        kernel_printf("Delete pde: %x\n", pde);
#endif //VMA_DEBUG 
        pde_ptr = (unsigned int*)pde;
        for (j = 0; j < 1024; j++) {
            pte = pde_ptr[j];
            pte &= PAGE_MASK;
            if (pte != 0) {
#ifdef VMA_DEBUG
                kernel_printf("Delete pte: %x\n", pte);
#endif //VMA_DEBUG 
                kfree((void*)pte);
            }
        }
        kfree((void*)pde_ptr);
    }  
    kfree(pgd);
#ifdef VMA_DEBUG
    kernel_printf("pgd_delete done\n");
#endif //VMA_DEBUG  
    return ;
}



/*************VMA*************/
// Find the first vma with ending address greater than addr
struct vma_struct* find_vma(struct mm_struct* mm, unsigned long addr)
{
    struct vma_struct* vma = 0;

    if (mm) {
        vma = mm->mmap_cache;
        if (vma && vma->vm_end>addr && vma->vm_start<=addr)
            return vma;
        vma = mm->mmap;
        while (vma) {
            if (vma->vm_end > addr) {
                mm->mmap_cache = vma;
                break;
            }
            vma = vma->vm_next;
        }
    }
    return vma;
}

// Find the first vma overlapped with start_addr~end_addr
struct vma_struct* find_vma_intersection(struct mm_struct* mm, unsigned long start_addr, unsigned long end_addr)
{
    struct vma_struct* vma = find_vma(mm, start_addr);
    if (vma && end_addr<=vma->vm_start)
        vma = 0;
    return vma;
}

// Return the first vma with ending address greater than addr, recording the previous vma
struct vma_struct* find_vma_and_prev(struct mm_struct* mm, unsigned long addr, struct vma_struct** prev)
{
    struct vma_struct* vma = 0;
    *prev = 0;
    if (mm) {
        vma = mm->mmap;
        while (vma) {
            if (vma->vm_end > addr) {
                mm->mmap_cache = vma;
                break;
            }
            *prev = vma;
            vma = vma->vm_next;
        }
    }
    return vma;
}

// Get unmapped area starting after addr        目前省略了file, pgoff
unsigned long get_unmapped_area(unsigned long addr, unsigned long len, unsigned long flags)
{
    struct vma_struct* vma;
    struct mm_struct* mm = current_task->mm;         //全局变量，当前线程对应的task_struct

    addr = UPPER_ALLIGN(addr, PAGE_SIZE);   // Allign to page size
    if (addr+len > KERNEL_ENTRY) return -1;
    
    if (addr && addr+len<=KERNEL_ENTRY) {
        for (vma=find_vma(mm, addr); ; vma=vma->vm_next) {
            if (addr+len > KERNEL_ENTRY) return -1;
            if (!vma || addr+len<=vma->vm_start) {
                return addr;
            }
            addr = vma->vm_end;
        }
    }

    return 0;
}

// Find vma preceding addr, assisting insertion
struct vma_struct* find_vma_prepare(struct mm_struct* mm, unsigned long addr)
{
    struct vma_struct* vma = mm->mmap;
    struct vma_struct* prev = 0;
    while (vma) {
        if (addr < vma->vm_start) break;
        prev = vma;
        vma = vma->vm_next;
    }
    return prev;
}

// Insert vma to the linked list
void insert_vma_struct(struct mm_struct* mm, struct vma_struct* area)
{
    struct vma_struct* vma = find_vma_prepare(mm, area->vm_start);
#ifdef VMA_DEBUG
    kernel_printf("Insert: %x  %x", vma, area->vm_start);
#endif //VMA_DEBUG
    if (!vma) {
#ifdef VMA_DEBUG
        kernel_printf("mmap init\n");
#endif //VMA_DEBUG
        mm->mmap = area;
        area->vm_next = 0;
    }
    else {
        area->vm_next = vma->vm_next;
        vma->vm_next = area;
    }
    mm->map_count ++;
}

// Mapping a region
unsigned long do_map(unsigned long addr, unsigned long len, unsigned long flags)
{
    struct mm_struct* mm = current_task->mm;
    struct vma_struct *vma, *prev;
    if (!len) return addr;
    addr = get_unmapped_area(addr, len, flags);
    vma = kmalloc(sizeof(struct vma_struct));
    if (!vma) return -1;

    vma->vm_mm = mm;
    vma->vm_start = addr;
    vma->vm_end = UPPER_ALLIGN(addr+len, PAGE_SIZE);
#ifdef VMA_DEBUG
    kernel_printf("Do map: %x  %x\n", vma->vm_start, vma->vm_end);
#endif //VMA_DEBUG
    insert_vma_struct(mm, vma);
    return addr;
}


int do_unmap(unsigned long addr, unsigned long len)
{
    struct mm_struct* mm = current_task->mm;
    struct vma_struct *vma, *prev;
    if (addr>KERNEL_ENTRY || len+addr>KERNEL_ENTRY) return -1;  // Bad addr
    vma = find_vma_and_prev(mm, addr, &prev);
    if (!vma || vma->vm_start>=addr+len) return 0;      // It has not been mapped
#ifdef VMA_DEBUG
    kernel_printf("do_unmap. %x %x\n", addr, vma->vm_start);
#endif //VMA_DEBUG
    // VMA Length match
    if (vma->vm_end >= addr+len) {
#ifdef VMA_DEBUG
        kernel_printf("Length match\n");
#endif //VMA_DEBUG
        if (!prev) {
            mm->mmap = vma->vm_next;
        }
        else {
            prev->vm_next = vma->vm_next;
        }
        kfree(vma);
        mm->map_count --;
#ifdef VMA_DEBUG
        kernel_printf("Unmap done. %d vma(s) left\n", mm->map_count);
#endif //VMA_DEBUG
        return 0;
    }
    
    // Length mismatch
#ifdef VMA_DEBUG
    kernel_printf("Length match");
#endif //VMA_DEBUG
    return 1;
}


// Free all the vmas
void exit_mmap(struct mm_struct* mm)
{
    struct vma_struct* vmap = mm->mmap;
    mm->mmap = mm->mmap_cache = 0;
    while (vmap) {
        struct vma_struct* next = vmap->vm_next;
        kfree(vmap);
        mm->map_count--;
        vmap = next;
    }
    if (mm->map_count) {
        kernel_printf("exit mmap bug! %d vma left", mm->map_count);
        //BUG
        while(1)    ;
    }
}


int is_in_vma(unsigned long addr)
{
    struct mm_struct* mm = current_task->mm;
    struct vma_struct *vma = find_vma(mm, addr);
    if (!vma || vma->vm_start>addr) return 0;
    else return 1;
}