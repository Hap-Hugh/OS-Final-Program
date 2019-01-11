#include <arch.h>
#include <driver/vga.h>
#include <zjunix/slab.h>
#include <zjunix/utils.h>

#define KMEM_ADDR(PAGE, BASE) ((((PAGE) - (BASE)) << PAGE_SHIFT) | 0x80000000)

/*
 * one list of PAGE_SHIFT(now it's 12) possbile memory size
 * 96, 192, 8, 16, 32, 64, 128, 256, 512, 1024, (2 undefined)
 * in current stage, set (2 undefined) to be (4, 2048)
 */
struct kmem_cache kmalloc_caches[PAGE_SHIFT];

static unsigned int size_kmem_cache[PAGE_SHIFT] = {96, 192, 8, 16, 32, 64, 128, 256, 512, 1024, 1536, 2048};

// init the struct kmem_cache_cpu
void init_kmem_cpu(struct kmem_cache_cpu *kcpu) {
    kcpu->page = 0;
}

// init the struct kmem_cache_node
void init_kmem_node(struct kmem_cache_node *knode) {
    INIT_LIST_HEAD(&(knode->full));
    INIT_LIST_HEAD(&(knode->partial));
}

void init_each_slab(struct kmem_cache *cache, unsigned int size) {
    cache->objsize = UPPER_ALLIGN(size, SIZE_INT);
    cache->size = cache->objsize + sizeof(void *);  // add one pointer to link free objects
    // cache->offset = cache->size;

    init_kmem_cpu(&(cache->cpu));
    init_kmem_node(&(cache->node));
}

void init_slab() {
    unsigned int i;

    for (i = 0; i < PAGE_SHIFT; i++) {
        init_each_slab(&(kmalloc_caches[i]), size_kmem_cache[i]);
    }
#ifdef SLAB_DEBUG
    kernel_printf("Setup Slub ok :\n");
    kernel_printf("\tcurrent slab cache size list:\n\t");
    for (i = 0; i < PAGE_SHIFT; i++) {
        kernel_printf("%x %x ", kmalloc_caches[i].objsize, (unsigned int)(&(kmalloc_caches[i])));
    }
    kernel_printf("\n");
#endif  // ! SLAB_DEBUG
}


void format_slabpage(struct kmem_cache *cache, struct page *page) {
    unsigned char *moffset = (unsigned char *)KMEM_ADDR(page, pages);  // physical addr

    set_flag(page, BUDDY_SLAB);

    struct slab_head *s_head = (struct slab_head *)moffset;
    void *ptr = moffset + sizeof(struct slab_head);

    s_head->end_ptr = ptr;
    s_head->nr_objs = 0;
    s_head->was_full = 0;

    cache->cpu.page = page;
    page->virtual = (void *)cache;      // Used in k_free
    page->slabp = 0;        // Point to the free-object list in this page
}

void *slab_alloc(struct kmem_cache *cache) {    
    void *object = 0;
    struct page *cur_page;
    struct slab_head *s_head;

    if (!cache->cpu.page) goto PageFull;

    cur_page = cache->cpu.page;
    s_head = (struct slab_head *)KMEM_ADDR(cur_page, pages);
#ifdef SLAB_DEBUG
    kernel_printf("was_full?:%d\n", s_head->was_full);
#endif 

FromFreeList:
    if (cur_page->slabp != 0) {     // Allocate from free list
        object = (void*)cur_page->slabp;
        cur_page->slabp = *(unsigned int*)cur_page->slabp;
        ++(s_head->nr_objs);
#ifdef SLAB_DEBUG
        kernel_printf("From Free-list\nnr_objs:%d\tobject:%x\tnew slabp:%x\n", 
                s_head->nr_objs, object, cur_page->slabp);
        // kernel_getchar();
#endif  // ! SLAB_DEBUG
        return object;
    }
    
PageNotFull:
    if (!s_head->was_full) {        // Still has uninitialized space
        object = s_head->end_ptr;
        s_head->end_ptr = object + cache->size;
        ++(s_head->nr_objs);
        
        if (s_head->end_ptr+cache->size - (void*)s_head >= 1<<PAGE_SHIFT) {
            s_head->was_full = 1;
            list_add_tail(&(cur_page->list), &(cache->node.full));
            
#ifdef SLAB_DEBUG
            kernel_printf("Become full\n");
            // kernel_getchar();
#endif  // ! SLAB_DEBUG
        }
#ifdef SLAB_DEBUG
        kernel_printf("Page not full\nnr_objs:%d\tobject:%x\tend_ptr:%x\n", 
        s_head->nr_objs, object, s_head->end_ptr);
        // kernel_getchar();
#endif  // ! SLAB_DEBUG
        return object;
    }

PageFull:
#ifdef SLAB_DEBUG
    kernel_printf("Page full\n");
    // kernel_getchar();
#endif  // ! SLAB_DEBUG

    if (list_empty(&(cache->node.partial))) {       // No partial pages
        // call the buddy system to allocate one more page to be slab-cache
        cur_page = __alloc_pages(0);  // get bplevel = 0 page === one page
        if (!cur_page) {
            // allocate failed, memory in system is used up
            kernel_printf("ERROR: slab request one page in cache failed\n");
            while (1)
                ;
        }
#ifdef SLAB_DEBUG
        // kernel_printf("\tnew page, index: %x \n", cur_page - pages);
#endif  // ! SLAB_DEBUG

        // using standard format to shape the new-allocated page,
        // set the new page to be cpu.page
        format_slabpage(cache, cur_page);
        s_head = (struct slab_head *)KMEM_ADDR(cur_page, pages);
        goto PageNotFull;
    }

    // Get a partial page
    #ifdef SLAB_DEBUG
    kernel_printf("Get partial page\n");
    #endif
    cache->cpu.page = container_of(cache->node.partial.next, struct page, list);
    cur_page = cache->cpu.page;
    list_del(cache->node.partial.next);
    s_head = (struct slab_head *)KMEM_ADDR(cur_page, pages);
    goto FromFreeList;
}

void slab_free(struct kmem_cache *cache, void *object) {
    struct page *opage = pages + ((unsigned int)object >> PAGE_SHIFT);
    unsigned int *ptr;
    struct slab_head *s_head = (struct slab_head *)KMEM_ADDR(opage, pages);
    unsigned char is_full;

    if (!(s_head->nr_objs)) {
        kernel_printf("ERROR : slab_free error!\n");
        // die();
        while (1)
            ;
    }
    object = (void*)((unsigned int)object | KERNEL_ENTRY);

#ifdef SLAB_DEBUG
    kernel_printf("page addr:%x\nobject:%x\nslabp:%x\n",
    opage, object, opage->slabp);
#endif  //SLAB_DEBUG

    is_full = (!opage->slabp) && s_head->was_full;
    *(unsigned int*)object = opage->slabp;
    opage->slabp = (unsigned int)object;
    --(s_head->nr_objs);
	
#ifdef SLAB_DEBUG
    kernel_printf("nr_objs:%d\tslabp:%x\n", s_head->nr_objs, opage->slabp);
    // kernel_getchar();
#endif  //SLAB_DEBUG

    if (list_empty(&(opage->list)))     // It's CPU
        return;
        
#ifdef SLAB_DEBUG
    kernel_printf("Not CPU\n");
#endif  //SLAB_DEBUG

    if (!(s_head->nr_objs)) {
        list_del_init(&(opage->list));
        __free_pages(opage, 0);         // 也可以不释放？
        return;
    #ifdef SLAB_DEBUG
        kernel_printf("Free\n");
    #endif  //SLAB_DEBUG
    }

    if (is_full) {
        list_del_init(&(opage->list));
        list_add_tail(&(opage->list), &(cache->node.partial));
    }    
}

// find the best-fit slab system for (size)
unsigned int get_slab(unsigned int size) {
    unsigned int itop = PAGE_SHIFT;
    unsigned int i;
    unsigned int bf_num = (1 << (PAGE_SHIFT - 1));  // half page
    unsigned int bf_index = PAGE_SHIFT;             // record the best fit num & index

    for (i = 0; i < itop; i++) {
        if ((kmalloc_caches[i].objsize >= size) && (kmalloc_caches[i].objsize < bf_num)) {
            bf_num = kmalloc_caches[i].objsize;
            bf_index = i;
        }
    }
    return bf_index;
}

void *kmalloc(unsigned int size) {
    void *result;

    if (!size)
        return 0;
    
    result = phy_kmalloc(size);

    if (result) return (void*)(KERNEL_ENTRY | (unsigned int)result);
    else return 0;
}

void *phy_kmalloc(unsigned int size) {
    struct kmem_cache *cache;
    unsigned int bf_index;

    if (!size)
        return 0;

    // if the size larger than the max size of slab system, then call buddy to
    // solve this
    if (size > kmalloc_caches[PAGE_SHIFT - 1].objsize) {
        size = UPPER_ALLIGN(size, 1<<PAGE_SHIFT);
        // size += (1 << PAGE_SHIFT) - 1;
        // size &= ~((1 << PAGE_SHIFT) - 1);
        return alloc_pages(size >> PAGE_SHIFT);
    }

    bf_index = get_slab(size);
    if (bf_index >= PAGE_SHIFT) {
        kernel_printf("ERROR: No available slab\n");
        while (1)
            ;
    }
    return slab_alloc(&(kmalloc_caches[bf_index]));
}

void kfree(void *obj) {
    struct page *page;

    obj = (void *)((unsigned int)obj & (~KERNEL_ENTRY));
    page = pages + ((unsigned int)obj >> PAGE_SHIFT);
    if (!(page->flag == BUDDY_SLAB))
        return free_pages((void *)((unsigned int)obj & ~((1 << PAGE_SHIFT) - 1)), page->bplevel);

    return slab_free(page->virtual, obj);
}
