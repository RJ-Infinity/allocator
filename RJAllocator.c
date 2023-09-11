#include <stddef.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <stdbool.h>

#define malloc RJ_malloc
#define calloc RJ_calloc
#define realloc RJ_realloc
#define free RJ_free

typedef unsigned char byte;

#ifndef RJ_MEM_SIZE
	#define RJ_MEM_SIZE (64 * 1000 * 1000)
#endif

byte _mem[RJ_MEM_SIZE] = {0};

#define book_keeper ((book_keep*)(&(_mem[RJ_MEM_SIZE-sizeof(book_keep)])))
#define block_to_header(b) ((alloc_header*)( ((byte*)(b)) - sizeof(alloc_header) ))
#define block_to_footer(b) ((alloc_footer*)(((byte*)(b)) + block_to_header(b)->size))
#define prev_header(b) (((alloc_footer*)( ((byte*)block_to_header(b)) - sizeof(alloc_footer) ))->header)
#define next_header(b) ((alloc_header*)(((byte*)(b)) + block_to_header(b)->size + sizeof(alloc_footer)))
#define header_to_block(h) ((block)(((byte*)(h)) + sizeof(alloc_header)))
#define first_header ((alloc_header*)&_mem[0])

typedef void* block;
typedef struct{
	block last;
} book_keep;
typedef struct{
	size_t size;
	bool isfree: 1;
} alloc_header;
typedef struct{ alloc_header* header; } alloc_footer;

#define dump_blocks() _dump_blocks(__FILE__, __LINE__)
void _dump_blocks(char* file, int line){
	printf("BLOCKS %s:%d\n", file, line);
	if (book_keeper->last == NULL){
		printf("    <no blocks>\n");
		return;
	}
	alloc_header* curr_header = first_header;
	while (true){
		printf("    block(%s) <%zu>\n", curr_header->isfree?"free":"used", curr_header->size);
		if (header_to_block(curr_header) == book_keeper->last) {return;}
		curr_header = next_header(header_to_block(curr_header));
	}
}

static block _create_last_block(size_t size){
	alloc_header* h = (book_keeper->last == NULL) ? first_header : next_header(book_keeper->last);

	if((byte*)h + sizeof(alloc_header) + size + sizeof(alloc_footer) > (byte*)book_keeper)
	{ return NULL; } // this means that there is not enough memory left for the block

	*h = (alloc_header){
		.size = size,
		.isfree = false,
	};
	book_keeper->last = header_to_block(h);
	*block_to_footer(book_keeper->last) = (alloc_footer){
		.header = h,
	};
	return book_keeper->last;
}

static block _split_block(alloc_header* h, size_t size){
	size_t oldSize = h->size;
	h->size = size;
	block new_b = header_to_block(next_header(header_to_block(h)));
	*block_to_header(new_b) = (alloc_header){
		.size = oldSize - size - sizeof(alloc_header) - sizeof(alloc_footer),
		.isfree = true,
	};
	*block_to_footer(new_b) = (alloc_footer){
		.header = block_to_header(new_b),
	};
	return new_b;
}

block malloc(size_t size){
	if (size == 0){ return NULL; }
	if (book_keeper->last == NULL) { return _create_last_block(size); }

	alloc_header* curr_header = first_header;
	while((char*)curr_header < (char*)&_mem + RJ_MEM_SIZE){
		if (curr_header->isfree && curr_header->size >= size){
			curr_header->isfree = false;
			assert(book_keeper->last != curr_header && "the last block should never be free as the last block should be moved backwards when freeing it");
			assert((next_header(header_to_block(curr_header))->isfree == false) && "as this is free the next block shouldnt be free or they would have been merged when freeing");
			if (curr_header->size > size + sizeof(alloc_header) + sizeof(alloc_footer)){
				// there is enough space to create a free block in the leftover space
				// when this isnt true we return a block that is too big
				_split_block(curr_header, size);
			}
			return header_to_block(curr_header);
		}else if(header_to_block(curr_header) == book_keeper->last) {
			return _create_last_block(size);
		} else { curr_header = next_header(header_to_block(curr_header)); }
	}
	return NULL;
}

block calloc(size_t size, size_t num){
	block rv = malloc(size * num);
	memset(rv, 0, size * num);
	return rv;
}

block realloc(block p, size_t newsize){
	(void)p;
	(void)newsize;
	return NULL;
}

static block _merge_prev(block b){
	if (block_to_header(b) == first_header) { return b; }
	alloc_header* prev_h = prev_header(b);
	if (!prev_h->isfree) { return b; }
	block_to_footer(b)->header = prev_h;
	prev_h->size += block_to_header(b)->size + sizeof(alloc_footer) + sizeof(alloc_header);
	return _merge_prev(header_to_block(prev_h));
}
static block _merge_next(block b){
	alloc_header* next = next_header(b);
	if (!next->isfree) { return b; }
	assert(header_to_block(next) != book_keeper->last && "the last element must not be free");
	alloc_header* head = block_to_header(b);
	head->size += next->size + sizeof(alloc_footer) + sizeof(alloc_header);
	block_to_footer(b)->header = head;
	return _merge_next(b);
}

void free(block p){
	alloc_header* p_head = block_to_header(p);
	assert((
		(block)p_head >= (block)_mem ||
		p_head->size + (byte*)p + sizeof(alloc_footer) > (byte*)book_keeper
	) && "this memory is outside the limit");

	p_head->isfree = true;

	bool is_last = p == book_keeper->last;

	p = _merge_prev(p);
	p_head = block_to_header(p);

	if (is_last){
		book_keeper->last = p_head == first_header ? NULL : header_to_block(prev_header(p));
	} else {
		_merge_next(p);
	}
}

int main(){
	dump_blocks();
	byte* x = malloc(10);
	dump_blocks();
	byte* y = malloc(2);
	dump_blocks();
	printf("_mem = %p\nx = %p\ny = %p\n", first_header, x, y);
	free(x);
	dump_blocks();
	free(y);
	dump_blocks();
	x = malloc(10);
	dump_blocks();
	printf("x = %p\n", x);

	return 0;
}
