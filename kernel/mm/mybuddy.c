#include <driver/vga.h>
#include <zjunix/bootmm.h>
#include <zjunix/buddy.h>
#include <zjunix/list.h>
#include <zjunix/lock.h>
#include <zjunix/utils.h>

unsigned int kernel_start_pfn, kernel_end_pfn;

struct page *pages;
struct buddy_sys buddy;

// Display buddy info, for debug
void buddy_info() {
    unsigned int index;
    kernel_printf("Buddy-system :\n");
    kernel_printf("\tstart page-frame number : %x\n", buddy.buddy_start_pfn);
    kernel_printf("\tend page-frame number : %x\n", buddy.buddy_end_pfn);
    for (index = 0; index <= MAX_BUDDY_ORDER; ++index) {
        kernel_printf("\t(%x)# : %x frees\n", index, buddy.freelist[index].nr_free);
    }
}

// Init page struct for all memory
void init_pages(unsigned int start_pfn, unsigned int end_pfn) {
    unsigned int i;
    for (i = start_pfn; i < end_pfn; i++) {
        set_flag(pages + i, BUDDY_RESERVED);
        (pages + i)->reference = 1;
        (pages + i)->virtual = (void *)(-1);
        (pages + i)->bplevel = 0;
        (pages + i)->slabp = 0;  // initially, the free space is the whole page
        INIT_LIST_HEAD(&(pages[i].list));
    }
}

// Init buddy system
void init_buddy() {
    unsigned int bpsize = sizeof(struct page);
    unsigned char *bp_base;
    unsigned int i;

    bp_base = bootmm_alloc_pages(bpsize * bmm.max_pfn, _MM_KERNEL, 1 << PAGE_SHIFT);
    if (!bp_base) {
        // Not enough memory to allocate the whole group
        // of buddy page struct
        kernel_printf("\nERROR : bootmm_alloc_pages failed!\nInit buddy system failed!\n");
        while (1)
            ;
    }

    // Get virtual address for pages array
    pages = (struct page *)((unsigned int)bp_base | 0x80000000);

    init_pages(0, bmm.max_pfn);

    kernel_start_pfn = 0;
    kernel_end_pfn = 0;
    for (i = 0; i < bmm.cnt_infos; ++i) {
        if (bmm.info[i].end > kernel_end_pfn)
            kernel_end_pfn = bmm.info[i].end;
    }
    kernel_end_pfn >>= PAGE_SHIFT;

    // Buddy system occupies the space after kernel part
    buddy.buddy_start_pfn = UPPER_ALLIGN(kernel_end_pfn, 1<<MAX_BUDDY_ORDER);
    // Remain 2 pages for I/O
    buddy.buddy_end_pfn = bmm.max_pfn & ~((1 << MAX_BUDDY_ORDER) - 1);

    // Init freelists of all bplevels
    for (i = 0; i < MAX_BUDDY_ORDER + 1; i++) {
        buddy.freelist[i].nr_free = 0;
        INIT_LIST_HEAD(&(buddy.freelist[i].free_head));
    }
    buddy.start_page = pages + buddy.buddy_start_pfn;
    init_lock(&(buddy.lock));
   

    for (i = buddy.buddy_start_pfn; i < buddy.buddy_end_pfn; ++i) {
        __free_pages(pages + i, 0);
    }
}


// Free pages. Partially free is not allowed
void __free_pages(struct page *pbpage, unsigned int bplevel) {
    /* page_idx -> the current page
     * pair_idx -> the pair of current page
     */
    unsigned int page_idx, pair_idx;
    unsigned int combined_idx, tmp;
    struct page *pair_page;

    lockup(&buddy.lock);

    page_idx = pbpage - buddy.start_page;

    while (bplevel < MAX_BUDDY_ORDER) {
        // Find pair to combine
        pair_idx = page_idx ^ (1 << bplevel);
        pair_page = pbpage + (pair_idx - page_idx);
        #ifdef BUDDY_DEBUG
        kernel_printf("group%x %x\n", (page_idx), pair_page);
        #endif
        
        if (!_is_same_bplevel(pair_page, bplevel)) {
            #ifdef BUDDY_DEBUG
            kernel_printf("%x %x\n", pair_page->bplevel, bplevel);
            #endif
            break;
        }

        if (pair_page->flag != BUDDY_FREE) {
            break;      // Its pair has been allocated or reserved
        }
        // Delete pair from freelist, and combine them
        list_del_init(&pair_page->list);
        --buddy.freelist[bplevel].nr_free;
        combined_idx = pair_idx & page_idx;
        // Set the bplevel of block being combined as -1
        if (combined_idx == pair_idx) set_bplevel(pair_page, -1);
        else set_bplevel(pbpage, -1);
        pbpage += (combined_idx - page_idx);
        page_idx = combined_idx;
        ++bplevel;
    }
    
    set_bplevel(pbpage, bplevel);
    
    set_flag(pbpage, BUDDY_FREE);
    
    // bug ???
    // struct list_head *new = &(pbpage->list);
    // struct list_head *prev = &(buddy.freelist[bplevel].free_head);
    // struct list_head *next = &(buddy.freelist[bplevel].free_head);
    // new->next = next;
    // new->prev = prev;
    // prev->next = new;
    // next->prev = new;

    list_add(&(pbpage->list), &(buddy.freelist[bplevel].free_head));
    #ifdef BUDDY_DEBUG
    kernel_printf("%x %x\n", pbpage->list.next, buddy.freelist[bplevel].free_head.next);
    // while (1);
    #endif
    ++buddy.freelist[bplevel].nr_free;
    
    unlock(&buddy.lock);
    
}

struct page *__alloc_pages(unsigned int bplevel) {
    unsigned int current_order, size;
    struct page *page, *buddy_page;
    struct freelist *freeL;

    lockup(&buddy.lock);

    // Search free pages
    for (current_order = bplevel; current_order <= MAX_BUDDY_ORDER; ++current_order) {
        freeL = buddy.freelist + current_order;
        if (!list_empty(&(freeL->free_head)))
            goto found;
    }

    // Not found
    unlock(&buddy.lock);
    return 0;

found:
    page = container_of(freeL->free_head.next, struct page, list);
    list_del_init(&(page->list));
    set_bplevel(page, bplevel);
    set_flag(page, BUDDY_ALLOCED);
    --(freeL->nr_free);

    size = 1 << current_order;
    while (current_order > bplevel) {
        --freeL;
        --current_order;
        size >>= 1;
        buddy_page = page + size;
        // Add the ramaining half into free list
        list_add(&(buddy_page->list), &(freeL->free_head));
        ++(freeL->nr_free);
        set_bplevel(buddy_page, current_order);
        set_flag(buddy_page, BUDDY_FREE);
    }

    unlock(&buddy.lock);
    return page;
}

void *alloc_pages(unsigned int nrPage) {
    unsigned int bplevel = 0;
    if (!nrPage)
        return 0;
    while (1<<bplevel < nrPage) {
        bplevel ++;
    }
    struct page *page = __alloc_pages(bplevel);

    if (!page)
        return 0;

    return (void *)((page - pages) << PAGE_SHIFT);
}

void free_pages(void *addr, unsigned int bplevel) {
    __free_pages(pages + ((unsigned int)addr >> PAGE_SHIFT), bplevel);
}
