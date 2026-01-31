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
#include <stdint.h>

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
bool	vector_reserve(Vector *v, size_t new_capacity);
void	vector_clear(Vector *v);
void	vector_destroy(Vector *v);
bool	vector_push(Vector *v, void *elem);
bool	vector_insert(Vector *v, size_t index, void *elem);
bool	vector_erase(Vector *v, size_t index);
bool	vector_pop(Vector *v);
bool	vector_shrink_to_fit(Vector *v);

/* ==================== */
/* -- Helper macros  -- */
/* ==================== */

/* vector_push_t(&vector, int, 42); */
#define vector_push_t(v, T, value) vector_push((v), &((T){value}))

/* vector_insert_t(&vector, int, 1, 42); */
#define vector_insert_t(v, T, idx, value) do { \
	T tmp = (value); \
	vector_insert((v), (idx), &tmp); \
} while (0)

/* int *arr = vector_data_as(&v, int); */
#define vector_data_as(v, T) ((T *)((v)->data))

/* int x = vector_at(&v, int, 0); */
#define vector_at(v, T, idx) ( \
	assert((idx) < (v)->size && "index out of bounds"), \
	vector_data_as(v, T)[idx] \
)

/* int last = vec_back(&v, int); */
#define vector_back(v, T) ( \
	assert((v)->size > 0 && "vector empty"), \
	vector_data_as(v, T)[(v)->size - 1] \
)

/* int first = vec_front(&v, int); */
#define vector_front(v, T) ( \
	assert((v)->size > 0 && "vector empty"), \
	vector_data_as(v, T)[0] \
)

/* Vector v = vector_init_malloc(int); */
#define vector_init_malloc(T) vector_init(malloc_allocator(), sizeof(T))

#endif // VEC_H

#ifdef VECTOR_IMPLEMENTATION
#ifndef VECTOR_IMPLEMENTATION_GUARD
#define VECTOR_IMPLEMENTATION_GUARD

/* ===================== */
/* -- Internal checks -- */
/* ===================== */
static inline bool vector_is_valid(const Vector *v)
{
	if (!v)
		return (false);
	if (v->size > v->capacity)
		return (false);
	if (v->capacity > 0 && !v->data)
		return (false);
	if (!v->alloc.alloc)
		return (false);
	if (v->elem_size == 0)
		return (false);
	return (true);
}

/* ==================== */
/* -- Initialization -- */
/* ==================== */
Vector vector_init(Allocator alloc, size_t elem_size)
{
	assert(alloc.alloc != NULL && "allocator must provide alloc function");
	assert(elem_size > 0 && "elem_size must be > 0");

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
bool vector_reserve(Vector *v, size_t new_capacity)
{
	assert(vector_is_valid(v) && "invalid vector");

	if (!v)
		return (false);
	if (new_capacity <= v->capacity)
		return (true);
	if (v->elem_size != 0 && new_capacity > SIZE_MAX / v->elem_size)
	{
		assert(0 && "vector_reserve: size overflow");
		return (false);
	}

	size_t alloc_size = new_capacity * v->elem_size;

	if (v->data && v->alloc.realloc)
	{
		void *p = v->alloc.realloc(v->alloc.ctx, v->data, alloc_size);
		if (!p)
		{
			assert(0 && "vector_reserve: realloc failed");
			return (false);
		}
		v->data = p;
	}
	else
	{
		void *new_block = v->alloc.alloc(v->alloc.ctx, alloc_size);
		if (!new_block)
		{
			assert(0 && "vector_reserve: alloc failed");
			return (false);
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
	return (true);
}

void vector_clear(Vector *v)
{
	assert(vector_is_valid(v) && "invalid vector");

	if (v)
		v->size = 0;
}

void vector_destroy(Vector *v)
{
	if (!v)
		return;

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

static bool grow_vector(Vector *v)
{
	size_t new_capacity;

	if (v->capacity == 0)
		new_capacity = 8;
	else if (v->capacity > SIZE_MAX / GROWTH_FACTOR)
		new_capacity = SIZE_MAX;
	else
		new_capacity = v->capacity * GROWTH_FACTOR;

	return (vector_reserve(v, new_capacity));
}

bool vector_push(Vector *v, void *elem)
{
	assert(vector_is_valid(v) && "invalid vector");
	assert(elem != NULL && "element is NULL");

	if (!v || !elem)
		return (false);

	if (v->size == v->capacity && !grow_vector(v))
		return (false);

	char *data = (char *)v->data;
	memcpy(data + v->size * v->elem_size, elem, v->elem_size);
	v->size++;
	return (true);
}

bool vector_insert(Vector *v, size_t index, void *elem)
{
	assert(vector_is_valid(v) && "invalid vector");
	assert(elem != NULL && "element is NULL");
	assert(index <= v->size && "index out of bounds");

	if (!v || !elem || index > v->size)
		return (false);
	if (v->size == v->capacity && !grow_vector(v))
		return (false);

	char *data = (char *)v->data;

	memmove(
		data + (index + 1) * v->elem_size,
		data + index * v->elem_size,
		(v->size - index) * v->elem_size
	);

	memcpy(data + index * v->elem_size, elem, v->elem_size);
	v->size++;
	return (true);
}

bool vector_erase(Vector *v, size_t index)
{
	assert(vector_is_valid(v) && "invalid vector");
	assert(index < v->size && "index out of bounds");

	if (!v || index >= v->size)
		return (false);

	char *data = (char *)v->data;

	memmove(
		data + index * v->elem_size,
		data + (index + 1) * v->elem_size,
		(v->size - index - 1) * v->elem_size
	);
	v->size--;
	return (true);
}

bool vector_pop(Vector *v)
{
	assert(vector_is_valid(v) && "invalid vector");
	assert(v->size > 0 && "vector is empty");

	if (!v || v->size == 0)
		return (false);

	v->size--;
	return (true);
}

bool vector_shrink_to_fit(Vector *v)
{
	assert(vector_is_valid(v) && "invalid vector");

	if (!v)
		return (false);
	if (v->size == v->capacity)
		return (true);

	if (v->size == 0)
	{
		if (v->alloc.free)
			v->alloc.free(v->alloc.ctx, v->data);
		v->data = NULL;
		v->capacity = 0;
		return (true);
	}

	size_t alloc_size = v->size * v->elem_size;

	if (v->alloc.realloc)
	{
		void *p = v->alloc.realloc(v->alloc.ctx, v->data, alloc_size);
		if (!p)
		{
			assert(0 && "vector_shrink_to_fit: realloc failed");
			return (false);
		}
		v->data = p;
	}
	else if (v->alloc.alloc)
	{
		void *new_block = v->alloc.alloc(v->alloc.ctx, v->size * v->elem_size);
		if (!new_block)
		{
			assert(0 && "vector_shrink_to_fit: alloc failed");
			return (false);
		}

		memcpy(new_block, v->data, v->size * v->elem_size);
		if (v->alloc.free)
			v->alloc.free(v->alloc.ctx, v->data);
		v->data = new_block;
	}
	v->capacity = v->size;
	return (true);
}

#endif // VECTOR_IMPLEMENTATION_GUARD
#endif // VECTOR_IMPLEMENTATION
