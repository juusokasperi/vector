/*
   -----------------------------------------------------------------------------
   VECTOR.H v1.0.0
   -----------------------------------------------------------------------------
   Memory-agnostic growable array implementation.
   
   Author:  Juuso Rinta
   Repo:    github.com/juusokasperi/memarena
   License: MIT
   -----------------------------------------------------------------------------
   
   USAGE:
     Define VECTOR_IMPLEMENTATION in *one* .c file before including this header.
     
     #define VECTOR_IMPLEMENTATION
     #include "vector.h"

	 In all other files, just #include "vector.h" as per normal.
*/

#ifndef VEC_H
#define VEC_H

#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>

#ifndef GROWTH_FACTOR
# define GROWTH_FACTOR (2)
#endif

typedef void *(*alloc_fn)(void *ctx, size_t size);
typedef void (*free_fn)(void *ctx, void *ptr);
typedef void *(*realloc_fn)(void *ctx, void *ptr, size_t size);


typedef struct {
	alloc_fn	alloc;
	realloc_fn	realloc;
	free_fn		free;
	void		*ctx;
} Allocator;

typedef struct {
	size_t			size;
	size_t			capacity;
	size_t			elem_size;
	void			*data;
	Allocator	alloc;
} Vector;

/* ================================== */
/* -- Allocator pattern for malloc -- */ 
/* ================================== */
static void *malloc_alloc(void *ctx, size_t size)
{
	(void)ctx;
	return (malloc(size));
}

static void *malloc_realloc(void *ctx, void *ptr, size_t size)
{
	(void)ctx;
	return (realloc(ptr, size));
}

static void malloc_free(void *ctx, void *ptr)
{
	(void)ctx;
	free(ptr);
}

static Allocator malloc_allocator(void)
{
	Allocator a;

	a.alloc = malloc_alloc;
	a.realloc = malloc_realloc;
	a.free = malloc_free;
	a.ctx = NULL;
	return (a);
}

/* ==================== */
/* -- API Prototypes -- */
/* ==================== */

// Initialization
Vector	vector_init(Allocator alloc, size_t elem_size);

// Modifiers
void	vector_reserve(Vector *v, size_t new_capacity);
void	vector_clear(Vector *v);
void	vector_destroy(Vector *v);
void	vector_push(Vector *v, void *elem);
void	vector_insert(Vector *v, size_t index, void *elem);
void	vector_erase(Vector *v, size_t index);
void	vector_pop(Vector *v);
void	vector_shrink_to_fit(Vector *v);

/* ==================== */
/* -- Helper macros  -- */
/* ==================== */

/* vector_push_t(&vector, int, 42); */
#define vector_push_t(v, T, value) do { \
	T tmp = (value); \
	vector_push((v), &tmp); \
} while(0)

/* vector_insert_t(&vector, int, 1, 42); */
#define vector_insert_t(v, T, idx, value) do { \
	T tmp = (value); \
	vector_insert((v), (idx), &tmp); \
} while (0)

/* int *arr = vector_data_as(&v, int); */
#define vector_data_as(v, T) ((T *)((v)->data))

/* int x = vector_at(&v, int, 0); */
#define vector_at(v, T, idx) (vector_data_as(v, T)[idx])

/* int last = vec_back(&v, int); */
#define vector_back(v, T) (vector_data_as(v, T)[(v)->size - 1])

/* int first = vec_front(&v, int); */
#define vector_front(v, T) (vector_data_as(v, T)[0])

/* Vector v = vector_init_malloc(int); */
#define vector_init_malloc(T) vector_init(malloc_allocator(), sizeof(T))

/* 
	int array[50]
	...fill the array..
	Vector v = vector_init_malloc(int);
	vector_from_array(&v, int, array, 50);
*/
#define vector_from_array(v, T, arr, count) do { \
	vector_reserve((v), (count)); \
	memcpy((v)->data, (arr), (count) * sizeof(T)); \
	(v)->size = (count); \
} while(0)

#ifdef VECTOR_IMPLEMENTATION_GUARD

/* ==================== */
/* -- Initialization -- */
/* ==================== */
Vector vector_init(Allocator alloc, size_t elem_size)
{
	Vector v;

	v.size = 0;
	v.capacity = 0;
	v.elem_size = elem_size;
	v.data = NULL;
	v.alloc = alloc;
	return (v);
}

/* ==================== */
/* -- Modifiers      -- */
/* ==================== */
void vector_reserve(Vector *v, size_t new_capacity)
{
	if (new_capacity <= v->capacity)
		return;

	if (v->data && v->alloc.realloc)
	{
		void *p = v->alloc.realloc(v->alloc.ctx, v->data, new_capacity * v->elem_size);
		if (!p)
			return;
		v->data = p;
	}
	else
	{
		void *new_block = v->alloc.alloc(v->alloc.ctx, new_capacity * v->elem_size);
		if (!new_block)
		{
			assert(0 && "vector_reserve: out of memory");
			return;
		}
		if (v->data)
		{
			memcpy(new_block, v->data, v->size * v->elem_size);
			if (v->alloc.free)
				v->alloc.free(v->alloc.ctx, v->data);
		}
		v->data = new_block;
	}

	v->capacity = new_capacity;
}

void vector_clear(Vector *v)
{
	v->size = 0;
}

void vector_destroy(Vector *v)
{
	if (v->data && v->alloc.free)
		v->alloc.free(v->alloc.ctx, v->data);
	v->size = 0;
	v->capacity = 0;
	v->elem_size = 0;
	v->data = NULL;
	v->alloc.alloc = NULL;
	v->alloc.free = NULL;
	v->alloc.realloc = NULL;
	v->alloc.ctx = NULL;
}

void vector_push(Vector *v, void *elem)
{
	if (v->size == v->capacity)
	{
		size_t new_capacity = v->capacity ? v->capacity * GROWTH_FACTOR : 8;
		vector_reserve(v, new_capacity);
	}

	char *data = (char *)v->data;
	size_t elem_size = v->elem_size;
	memcpy(data + v->size * elem_size, elem, elem_size);
	v->size++;
}

void vector_insert(Vector *v, size_t index, void *elem)
{
	assert(index <= v->size);

	if (v->size == v->capacity)
	{
		size_t new_capacity = v->capacity ? v->capacity * GROWTH_FACTOR : 8;
		vector_reserve(v, new_capacity);
	}

	char *data = (char *)v->data;
	size_t elem_size = v->elem_size;

	memmove(
		data + (index + 1) * elem_size,
		data + index * elem_size,
		(v->size - index) * elem_size
	);

	memcpy(data + index * elem_size, elem, elem_size);
	v->size++;
}

void vector_erase(Vector *v, size_t index)
{
	assert (index < v->size);

	char *data = (char *)v->data;
	size_t elem_size = v->elem_size;

	memmove(
		data + index * elem_size,
		data + (index + 1) * elem_size,
		(v->size - index - 1) * elem_size
	);
	v->size--;
}

void vector_pop(Vector *v)
{
	if (v->size > 0)
		vector_erase(v, v->size - 1);
}

void vector_shrink_to_fit(Vector *v)
{
	if (v->size == v->capacity)
		return;

	if (v->size == 0)
	{
		if (v->alloc.free)
			v->alloc.free(v->alloc.ctx, v->data);
		v->data = NULL;
		v->capacity = 0;
		return;
	}
	if (v->alloc.realloc)
	{
		void *p = v->alloc.realloc(v->alloc.ctx, v->data, v->size * v->elem_size);
		if (!p)
			return;
		v->data = p;
	}
	else if (v->alloc.alloc)
	{
		void *new_block = v->alloc.alloc(v->alloc.ctx, v->size * v->elem_size);
		if (!new_block)
		{
			assert(0 && "vector_shrink_to_fit: out of memory");
			return;
		}

		memcpy(new_block, v->data, v->size * v->elem_size);
		if (v->alloc.free)
			v->alloc.free(v->alloc.ctx, v->data);
		v->data = new_block;
	}
	v->capacity = v->size;
}

#endif // VECTOR_IMPLEMENTATION_GUARD

#endif // VEC_H
