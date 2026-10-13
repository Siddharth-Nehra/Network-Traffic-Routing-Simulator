#ifndef NETSIM_HEAP_H
#define NETSIM_HEAP_H

#include "common.h"

typedef struct HeapItem {
    double key;
    int value;
    void *ptr;
} HeapItem;

typedef struct Heap {
    HeapItem *data;
    int size;
    int cap;
} Heap;

/* Initialize an empty heap. Returns 0 on success. */
int heap_init(Heap *h);

/* Release heap storage. */
void heap_free(Heap *h);

/* Insert an item. Returns 0 on success, -1 on allocation failure. */
int heap_push(Heap *h, double key, int value, void *ptr);

/* Remove the minimum item. Returns 0 if an item was written to out. */
int heap_pop(Heap *h, HeapItem *out);

/* Peek at the minimum without removing it. Returns 0 on success. */
int heap_peek(const Heap *h, HeapItem *out);

/* Return 1 if the heap has no items. */
int heap_empty(const Heap *h);

/* Return the number of items. */
int heap_size(const Heap *h);

/* Remove all items but keep allocated capacity. */
void heap_clear(Heap *h);

#endif /* NETSIM_HEAP_H */
