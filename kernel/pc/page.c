#include <zjunix/page.h>
#include <zjunix/slab.h>
#include <zjunix/utils.h>
#include <driver/vga.h>

//将物理地址pa所指向的页映射到从虚拟地址va开始的页, pgd为映射所发生的页表
//attr表示映射的页所具有的属性
int do_one_mapping(pgd_t *pgd, unsigned int va, unsigned int pa, unsigned int attr) {
    unsigned int pde_index, pte_index;
    unsigned int pde, *pt;

    //分别获取两级页表的索引
    pde_index = va >> PGD_SHIFT;
    pte_index = (va >> PAGE_SHIFT) & INDEX_MASK;

    //对一级页表进行检索
    pde = pgd[pde_index];
    pde &= PAGE_MASK;

    if (pde == 0) {  
        //二级页表不存在，则分配
        pde = (unsigned int) kmalloc(PAGE_SIZE);
        if (pde == 0)
            goto err_alloc;
        
        kernel_memset((void*)pde, 0, PAGE_SIZE);

        pgd[pde_index] = pde;
        pgd[pde_index] &= PAGE_MASK;
        pgd[pde_index] |= attr;
    } else {
        pgd[pde_index] &= PAGE_MASK;
        pgd[pde_index] |= attr;
    }

    pt = (unsigned int*)pde;

    #ifdef VMA_DEBUG
    kernel_printf("Map va:%x  pa:%x\npde_index:%x  pde:%x\n", va, pa, pde_index, pde);
    #endif

    //将物理地址pa插入二级页表相应项中
    pt[pte_index] = pa & PAGE_MASK;
    pt[pte_index] |= attr;

    return 0;

err_alloc:
    return 1;
}

//同时对pa所指向的npage个物理页进行虚拟映射，映射的起始虚拟地址为va
int do_mapping(pgd_t *pgd, unsigned int va, unsigned int npage, unsigned int pa, unsigned int attr) {
    int i, j, res;

    for (i = 0; i < npage; i ++) {
        //循环对单个物理页进行映射
        res = do_one_mapping(pgd, va, pa, attr);
        if (res != 0)
            goto error;
        
        //同时递增虚拟、物理地址
        va += PAGE_SIZE; 
        pa += PAGE_SIZE;
    }

    return 0;

error:
    return 1;
}
