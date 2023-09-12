#include <stdio.h>
#include "RJAllocator.h"

int main(){
	void* a;
	void* b;
	// void* c;

	printf("basic malloc and free ================================================\n");
	                        dump_blocks();
	a = malloc(10);         dump_blocks();
	b = malloc(2);          dump_blocks();
	free(a);                dump_blocks();
	free(b);                dump_blocks();
	a = malloc(10);         dump_blocks();
	free(a);                dump_blocks();

	printf("realloc last block smaller ===========================================\n");
	                        dump_blocks();
	a = malloc(10);         dump_blocks();
	a = realloc(a, 5);      dump_blocks();
	free(a);                dump_blocks();

	printf("realloc block smaller and create free new block ======================\n");
	                        dump_blocks();
	// the size between blocks is 12 so allocate a bit more than that
	a = malloc(16);         dump_blocks();
	// then allocate a new block so it dosent just reposition the end of the used memory
	b = malloc(10);         dump_blocks();
	a = realloc(a, 2);      dump_blocks();
	free(a);                dump_blocks();
	free(b);                dump_blocks();

	printf("realloc block smaller without space for new block ====================\n");
	                        dump_blocks();
	// the size between blocks is 12 so allocate a bit less than that
	a = malloc(10);         dump_blocks();
	// then allocate a new block so it dosent just reposition the end of the used memory
	b = malloc(10);         dump_blocks();
	a = realloc(a, 2);      dump_blocks();
	free(a);                dump_blocks();
	free(b);                dump_blocks();

	printf("realloc last block larger ============================================\n");
	                        dump_blocks();
	a = malloc(10);         dump_blocks();
	realloc(a, 20);         dump_blocks();
	free(a);                dump_blocks();

	printf("realloc block larger with no space ===================================\n");
	                        dump_blocks();
	a = malloc(10);         dump_blocks();
	b = malloc(10);         dump_blocks();
	a = realloc(a, 20);     dump_blocks();
	free(b);                dump_blocks();
	free(a);                dump_blocks();

	return 0;
}
