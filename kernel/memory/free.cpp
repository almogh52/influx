#include <stddef.h>
#include <kernel/memory/heap.h>

void operator delete(void *p)
{
    kfree(p);
}
 
void operator delete[](void *p)
{
    kfree(p);
}