#include <stdio.h>
#include "RJAllocator.h"

int main(){
	dump_blocks();
	void* x = malloc(10);
	dump_blocks();
	void* y = malloc(2);
	dump_blocks();
	printf("x = %p\ny = %p\n", x, y);
	free(x);
	dump_blocks();
	free(y);
	dump_blocks();
	x = malloc(10);
	dump_blocks();
	free(x);
	dump_blocks();
	printf("x = %p\n", x);

	return 0;
}
