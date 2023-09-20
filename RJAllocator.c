#include "RJAllocator.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#ifdef DEBUG
	#include <assert.h>
	#define _return_void_assert(cond) assert(cond)
	#define _return_val_assert(cond, val) _return_void_assert(cond)
#else 
	#define _return_void_assert(cond) if(!(cond)){return;}
	#define _return_val_assert(cond, val) if(!(cond)){return val;}
#endif

// this macro trick is taken from https://stackoverflow.com/a/3048361/15755351
#define GET_3RD_ARG(arg1, arg2, arg3, ...) arg3
#define return_assert(...) GET_3RD_ARG(__VA_ARGS__, _return_val_assert, _return_void_assert, )(__VA_ARGS__)

typedef unsigned char byte;

byte _mem[RJ_MEM_SIZE] = {0};

#ifdef DONT_USE_END
	#define book_keeper ((book_keep*)&_mem[0])
	#define first_header ((alloc_header*)((byte*)book_keeper + sizeof(book_keep)))
	#define mem_end ((byte*)&_mem + RJ_MEM_SIZE)
#else
	#define book_keeper ((book_keep*)(&(_mem[RJ_MEM_SIZE-sizeof(book_keep)])))
	#define first_header ((alloc_header*)&_mem[0])
	#define mem_end ((byte*)book_keeper)
#endif

#define block_to_header(b) ((alloc_header*)( ((byte*)(b)) - sizeof(alloc_header) ))
#define block_to_footer(b) ((alloc_footer*)(((byte*)(b)) + block_to_header(b)->size))
#define prev_header(b) (((alloc_footer*)( ((byte*)block_to_header(b)) - sizeof(alloc_footer) ))->header)
#define next_header(b) ((alloc_header*)(((byte*)(b)) + block_to_header(b)->size + sizeof(alloc_footer)))
#define header_to_block(h) ((block)(((byte*)(h)) + sizeof(alloc_header)))

typedef void* block;
typedef struct{
	block last;
} book_keep;
typedef struct{
	size_t size;
	bool isfree: 1;
} alloc_header;
typedef struct{ alloc_header* header; } alloc_footer;

void _dump_blocks(char* file, int line){
	printf("BLOCKS %s:%d\n", file, line);
	printf("    from memory at 0x%p\n", first_header);
	if (book_keeper->last == NULL){
		printf("    <no blocks>\n");
		return;
	}
	alloc_header* curr_header = first_header;
	while (true){
		printf(
			"    block(%s) at 0x%p <%zu>\n",
			curr_header->isfree?"free":"used",
			header_to_block(curr_header),
			curr_header->size
		);
		if (header_to_block(curr_header) == book_keeper->last) {return;}
		curr_header = next_header(header_to_block(curr_header));
	}
}

static block _create_last_block(size_t size){
	alloc_header* h = (book_keeper->last == NULL) ? first_header : next_header(book_keeper->last);

	return_assert(
		(byte*)h + sizeof(alloc_header) + size + sizeof(alloc_footer) <= (byte*)(mem_end) &&
		"this means that there is not enough memory left for the block", NULL
	);

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

static block _split_block(alloc_header* h, size_t new_size){
	return_assert(
		h->size > new_size + sizeof(alloc_header) + sizeof(alloc_footer) &&
		"there isnt enough space to create a free block in the leftover space so fail", NULL
	);
	size_t oldSize = h->size;
	h->size = new_size;
	*block_to_footer(header_to_block(h)) = (alloc_footer){.header = h,};

	block new_b = header_to_block(next_header(header_to_block(h)));
	*block_to_header(new_b) = (alloc_header){
		.size = oldSize - new_size - sizeof(alloc_header) - sizeof(alloc_footer),
		.isfree = true,
	};
	*block_to_footer(new_b) = (alloc_footer){
		.header = block_to_header(new_b),
	};
	return new_b;
}

block malloc(size_t size){
	return_assert(size != 0, NULL);
	if (book_keeper->last == NULL) { return _create_last_block(size); }

	alloc_header* curr_header = first_header;
	while((byte*)curr_header < mem_end){
		if (curr_header->isfree && curr_header->size >= size){
			curr_header->isfree = false;
			return_assert(book_keeper->last != curr_header && "the last block should never be free as the last block should be moved backwards when freeing it", NULL);
			return_assert((next_header(header_to_block(curr_header))->isfree == false) && "as this is free the next block shouldnt be free or they would have been merged when freeing", NULL);
			_split_block(curr_header, size);
			return header_to_block(curr_header);
		}else if(header_to_block(curr_header) == book_keeper->last) {
			return _create_last_block(size);
		} else { curr_header = next_header(header_to_block(curr_header)); }
	}
	return_assert("this means there is no space left", NULL);
}

block calloc(size_t size, size_t num){
	block rv = malloc(size * num);
	memset(rv, 0, size * num);
	return rv;
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
	return_assert(header_to_block(next) != book_keeper->last && "the last element must not be free", NULL);
	alloc_header* head = block_to_header(b);
	head->size += next->size + sizeof(alloc_footer) + sizeof(alloc_header);
	block_to_footer(b)->header = head;
	return _merge_next(b);
}

void safe_memcpy(byte* dest, byte* src, size_t count)
{for (size_t i = 0; i < count; i++) { dest[i] = src[i]; }}

block realloc(block p, size_t newsize){
	if (p == NULL) {return malloc(newsize);}
	if (newsize == 0){
		free(p);
		return NULL;
	}
	alloc_header* p_head = block_to_header(p);
	if (p_head->size == newsize){return p;}
	return_assert((
		(block)p_head >= (block)first_header &&
		(block)next_header(p) < (block)(mem_end)
	) && "this memory is outside the limit", NULL);
	if (p_head->size >= newsize){
		if (p == book_keeper->last){
			p_head->size = newsize;
			block_to_footer(p)->header = p_head;
		}else{
			block new_b = _split_block(p_head, newsize);
			if (new_b != NULL){ _merge_next(new_b); }
		}
		return p;
	}
	if (p == book_keeper->last){
		if (((byte*)p) + newsize + sizeof(alloc_footer) < (byte*)(mem_end)) {
			p_head->size = newsize;
			block_to_footer(p)->header = p_head;
			return p;
		}
	}else if(next_header(p)->isfree){
		return_assert(next_header(header_to_block(next_header(p)))->isfree == false && "there should only be one free block in a row", NULL);
		if (p_head->size + sizeof(alloc_footer) + sizeof(alloc_header) + next_header(p)->size >= newsize){
			block new_b = _split_block(
				next_header(p),
				newsize - p_head->size - sizeof(alloc_footer) - sizeof(alloc_header)
			);
			block_to_footer(header_to_block(next_header(p)))->header = p_head;
			if (new_b == NULL){ // this means that the block is slightly to big so we
				// need to calculate the larger size
				p_head->size += sizeof(alloc_footer) + sizeof(alloc_header) + next_header(p)->size;
			}else{ p_head->size = newsize; }
			return p;
		}
	}
	// we cant expand the old block so we have to copy the contents 
	return_assert(p_head->size < newsize && "this should be handled earlier", NULL);
	if (p_head != first_header && prev_header(p)->isfree){
		block prev_b = header_to_block(prev_header(p));

		size_t surounding_space = prev_header(p)->size + sizeof(alloc_footer) + sizeof(alloc_header) + p_head->size;
		if (p != book_keeper->last && next_header(p)->isfree)
		{surounding_space += sizeof(alloc_footer) + sizeof(alloc_header) + next_header(p)->size;}
		
		if (surounding_space >= newsize){
			prev_header(p)->isfree = false;
			prev_header(p)->size = surounding_space;
			block_to_footer(header_to_block(prev_header(p)))->header = prev_header(p);
			// cant use memcpy as they are potentialy overlapping
			safe_memcpy(prev_b, p, p_head->size);
			block new_b = _split_block(prev_header(p), newsize);
			if (new_b != NULL){ _merge_next(new_b); }
			return prev_b;
		}
	}
	//there is no way of merging the current block so get a completly new block and free the old one
	block new_b = malloc(newsize);
	if (new_b != NULL){
		memcpy(new_b, p, p_head->size);
		free(p);
	}
	return new_b;
}

void free(block p){
	return_assert(p != NULL);
	alloc_header* p_head = block_to_header(p);
	return_assert((
		(block)p_head >= (block)first_header &&
		(block)next_header(p) < (block)(mem_end) &&
		p <= book_keeper->last
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
