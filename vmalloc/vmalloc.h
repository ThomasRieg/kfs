#ifndef VMALLOC_H
#define VMALLOC_H

#include <stddef.h>
#include <stdbool.h>

virt_ptr vmalloc(uint32_t size);
void vfree(virt_ptr ptr);
virt_ptr vcalloc(uint32_t nmemb, uint32_t size);
virt_ptr vrealloc(virt_ptr ptr, uint32_t size);
uint32_t vsize(virt_ptr ptr);

#endif