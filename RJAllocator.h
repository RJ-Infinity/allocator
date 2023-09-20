#pragma once

#include <stddef.h>

#ifndef REDEFINE_MALLOC_WITH_RJ_IMPLEMETATIONS
	#define malloc RJ_malloc
	#define calloc RJ_calloc
	#define realloc RJ_realloc
	#define free RJ_free
	#define malloc_usable_size RJ_malloc_usable_size
#endif

#define ENOMEM 12 // no memory left on the system
#define	EINVAL 22 // invalid argument
#define	EFAULT 14 // invalid memory
#define ESTATE 35 // an invalid state (usualy due to an incorect argument passed the system is in an invalid state restart the program)

#ifndef RJ_MEM_SIZE
	#define RJ_MEM_SIZE (64 * 1000 * 1000)
#endif

#define dump_blocks() _dump_blocks(__FILE__, __LINE__)
void _dump_blocks(char* file, int line);
size_t malloc_usable_size(void* p);

void* malloc(size_t size);
void* calloc(size_t size, size_t num);
void* realloc(void* p, size_t newsize);
void free(void* p);