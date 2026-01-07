#include <stdint.h>
#include <stddef.h>

typedef _Bool bool;
#define false 0
#define true 1

typedef void *virt_ptr;
typedef uint32_t phys_ptr;

void kernel_panic(const char *message);
