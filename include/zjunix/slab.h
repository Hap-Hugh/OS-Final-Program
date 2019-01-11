#ifndef _ZJUNIX_SLAB_H
#define _ZJUNIX_SLAB_H

#include <zjunix/list.h>
#include <zjunix/buddy.h>

#define SIZE_INT 4
#define SLAB_AVAILABLE 0x0
#define SLAB_USED 0xff

/*
 * slab_head makes the allocation accessible from end_ptr to the end of the page
 * @end_ptr : points to the head of the rest of the page
 * @nr_objs : keeps the numbers of memory segments that has been allocated
 */
struct slab_head {
    void *end_ptr;
    unsigned int nr_objs;
    char was_full;
};

/*
 * slab pages is chained in this struct
 * @partial keeps the list of un-totally-allocated pages
 * @full keeps the list of totally-allocated pages
 */
struct kmem_cache_node {
    struct list_head partial;
    struct list_head full;
};

// Current-page-used
struct kmem_cache_cpu {
    struct page *page;
};

struct kmem_cache {
    unsigned int size;              // Total size of a slab
    unsigned int objsize;           // Object size
    struct kmem_cache_node node;    // Partial and full lists
    struct kmem_cache_cpu cpu;      // Current page
};


extern void init_slab();
extern void *kmalloc(unsigned int size);
extern void *phy_kmalloc(unsigned int size);
extern void kfree(void *obj);

#endif
