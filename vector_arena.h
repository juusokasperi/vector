/*
   -----------------------------------------------------------------------------
   VECTOR_ARENA.H v1.0.0
   -----------------------------------------------------------------------------
   Helper header file for easy patching between my memarena.h and vector.h.
   
   Author:  Juuso Rinta
   Repo:    github.com/juusokasperi/vector & github.com/juusokasperi/memarena
   License: MIT
   -----------------------------------------------------------------------------
   
   USAGE:
	 Arena arena = arena_init(PROT_READ | PROT_WRITE);
	 Vector v = vector_init(arena_allocator(&arena), sizeof(int));
*/

#ifndef VEC_ARENA_H
# define VEC_ARENA_H

#include "vector.h"
#include "memarena.h"

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
