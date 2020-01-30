#include <stddef.h>
#include <kernel/memory/heap.h>
 
void *operator new(size_t size)
{
    return kmalloc(size);
}
 
void *operator new[](size_t size)
{
    return kmalloc(size);
}