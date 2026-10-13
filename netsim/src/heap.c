#include "heap.h"

/* Grow the heap array to at least new_cap items. */
static int heap_reserve(Heap *h, int new_cap)
{
    HeapItem *grown;
    if (new_cap <= h->cap) {
        return 0;
    }
    grown = (HeapItem *)realloc(h->data, (size_t)new_cap * sizeof(HeapItem));
    if (!grown) {
        return -1;
    }
    h->data = grown;
    h->cap = new_cap;
    return 0;
}

int heap_init(Heap *h)
{
    if (!h) {
        return -1;
    }
    h->data = NULL;
    h->size = 0;
    h->cap = 0;
    return heap_reserve(h, 16);
}

void heap_free(Heap *h)
{
    if (!h) {
        return;
    }
    free(h->data);
    h->data = NULL;
    h->size = 0;
    h->cap = 0;
}

/* Swap two heap items. */
static void heap_swap(HeapItem *a, HeapItem *b)
{
    HeapItem tmp = *a;
    *a = *b;
    *b = tmp;
}

/* Restore the heap property toward the root. */
static void heap_sift_up(Heap *h, int i)
{
    while (i > 0) {
        int parent = (i - 1) / 2;
        if (h->data[i].key >= h->data[parent].key) {
            break;
        }
        heap_swap(&h->data[i], &h->data[parent]);
        i = parent;
    }
}

/* Restore the heap property toward the leaves. */
static void heap_sift_down(Heap *h, int i)
{
    for (;;) {
        int left = 2 * i + 1;
        int right = 2 * i + 2;
        int smallest = i;
        if (left < h->size && h->data[left].key < h->data[smallest].key) {
            smallest = left;
        }
        if (right < h->size && h->data[right].key < h->data[smallest].key) {
            smallest = right;
        }
        if (smallest == i) {
            break;
        }
        heap_swap(&h->data[i], &h->data[smallest]);
        i = smallest;
    }
}

int heap_push(Heap *h, double key, int value, void *ptr)
{
    HeapItem *slot;
    if (!h) {
        return -1;
    }
    if (h->size >= h->cap) {
        int cap = h->cap < 16 ? 16 : h->cap * 2;
        if (heap_reserve(h, cap) != 0) {
            return -1;
        }
    }
    slot = &h->data[h->size];
    slot->key = key;
    slot->value = value;
    slot->ptr = ptr;
    h->size += 1;
    heap_sift_up(h, h->size - 1);
    return 0;
}

int heap_pop(Heap *h, HeapItem *out)
{
    if (!h || !out || h->size <= 0) {
        return -1;
    }
    *out = h->data[0];
    h->size -= 1;
    if (h->size > 0) {
        h->data[0] = h->data[h->size];
        heap_sift_down(h, 0);
    }
    return 0;
}

int heap_peek(const Heap *h, HeapItem *out)
{
    if (!h || !out || h->size <= 0) {
        return -1;
    }
    *out = h->data[0];
    return 0;
}

int heap_empty(const Heap *h)
{
    return !h || h->size <= 0;
}

int heap_size(const Heap *h)
{
    return h ? h->size : 0;
}

void heap_clear(Heap *h)
{
    if (h) {
        h->size = 0;
    }
}
