# allocator

a small allocator that i wrote mainly to explore how allocators work

it contains implimentations for `malloc`, `calloc`, `realloc`, `free` and also `dump_blocks` which shows all the blocks of memory.

if you compile with the `DEBUG` flag it never fails by returning NULL instead it fails an assertion

if you compile with the `DONT_USE_END` flag it store bookkeeping at the begining of the memory not the end

you can change how much memory it has with the `RJ_MEM_SIZE` flag

i would imagine that it could be usefull for an embeded device where you can set the _mem var to the memory of the device however i havent tested it like that so it might have bugs
