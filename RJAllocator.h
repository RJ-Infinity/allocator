#pragma once

#include <stddef.h>

#define malloc RJ_malloc
#define calloc RJ_calloc
#define realloc RJ_realloc
#define free RJ_free

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