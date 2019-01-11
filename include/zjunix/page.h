#ifndef  _ZJUNIX_PAGH_H
#define  _ZJUNIX_PAGE_H

#define  PAGE_SHIFT   12     //size of page
#define  PAGE_SIZE    (1 << PAGE_SHIFT)
#define  PAGE_MASK    (~(PAGE_SIZE - 1))

#define  INDEX_MASK    0x3ff  //低10位
#define  PGD_SHIFT     22
#define  PGD_SIZE     (1 << PAGE_SHIFT)
#define  PGD_MASK     (~((1 << PGD_SHIFT) - 1))


typedef unsigned int pgd_t;
typedef unsigned int pte_t;

int do_one_mapping(pgd_t *pgd, unsigned int va, unsigned int pa, unsigned int attr);
int do_mapping(pgd_t *pgd, unsigned int va, unsigned int npage, unsigned int pa, unsigned int attr);

#endif