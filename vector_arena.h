#ifndef VEC_ARENA_H
# define VEC_ARENA_H

#include "vector.h"
#include "memarena.h"

/*
*	Helper header file
*	Allows for easy use of vector.h 
*	with memarena.h (https://www.github.com/juusokasperi/memarena)
*
* 	Example usage;
*	Arena arena = arena_init(PROT_READ | PROT_WRITE);
*	Vector v = vector_init(arena_allocator(&arena), sizeof(int));
*/

static void *arena_alloc_wrapper(void *ctx, size_t size)
{
	return (arena_alloc((Arena *)ctx, size));
}

static Allocator arena_allocator(Arena *arena)
{
	Allocator a;

	a.alloc = arena_alloc_wrapper;
	a.realloc = NULL;
	a.free = NULL;
	a.ctx = arena;
	return (a);
}

#endif // VEC_ARENA_H
