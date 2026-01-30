# Vector implementation for C

A small, fast, allocator-agnostic growable array implementation for C, inspired by std::vector, designed to integrate cleanly with custom allocators such as memory arenas.

## Features

- Dynamic resizing array (like std::vector)
- Allocator abstraction (plug in malloc, arenas, pools, etc.)
- Fast contiguous storage
- Header-only (single-header implementation)
- Easy integration with memarena
- Clean and minimal API
- Zero dependencies

## Installation

1. Copy vector.h into your project.
2. In **one** .c file (e.g. `main.c`) define `VECTOR_IMPLEMENTATION` *before* including the header:
```c
#define VECTOR_IMPLEMENTATION
#include "vector.h"
```

3. In all other files, just include the header file:
```
#include "vector.h"
```

## Usage

### Creating a vector
```c
// If using malloc;
Vector v = vector_init(malloc_allocator(), int);
// Or simpler:
Vector v = vector_init_malloc(int);

// To reserve (pre-allocate) space in the array
vector_reserve(&v, 1024);
```

### Pushing values
```c
int x = 42;
vector_push(&v, x);
// Or push_t, which allows for pushing elements
// that are not initialized variables
vector_push_t(&v, int, 42);
vector_push_t(&v, int, 1337);

// Insert an element to a certain index
vector_insert(&v, 0, x);
// Or with insert_t like as above
vector_insert_t(&v, int, 0, 42);
```

### Accessing values
```c
int x = vector_at(&v, int, 0);
int last = vector_back(&v, int);
int first = vector_front(&v, int);
```

### Iteration
```c
// Option 1 (iterating through an integer vector)
for (int i = 0; i < v->size; ++i)
    printf("%d ", vector_at(&v, int, 0);
// Option 2
char *data = vector_data_as(&v, char*);
for (int i = 0; i < v->size; ++i)
    printf("%s ", data[i]);
```

### Removing elements
```c
vector_pop(&v);
vector_erase(&v, 0);
```

### Shrink to Fit
To remove unused capacity from the vector.
```c
Vector v = vector_init_malloc(int);
vector_reserve(&v, 1024);
vector_push_t(&v, int, 42);
vector_shrink_to_fit(&v);
// Now v->size == 1 && v->capacity == 1
```

### Cleanup
```c
// Clear just moves the size pointer of the vector back to index 0
vector_clear(&v);
// Destroy clears the whole array and frees if the memory allocator supports it.
vector_destroy(&v);
```

### Using Custom Allocators

The vector is fully allocator-agnostic.

### Allocator interface
```c
typedef void *(*alloc_fn)(void *ctx, size_t size);
typedef void *(*realloc_fn)(void *ctx, void *ptr, size_t size);
typedef void (*free_fn)(void *ctx, void *ptr);

typedef struct {
    alloc_fn    alloc;
    realloc_fn  realloc;
    free_fn     free;
    void       *ctx;
} Allocator;
```

You can plug in any allocator model, e.g. **memory arenas**, **region allocators**, **pool allocators** or **custom tracking allocators**.

### Integration with my [memarena](https://www.github.com/juusokasperi/memarena/)

If you use memarena, include the helper:
```c
#include "vector_arena.h"
```

Example:
```c
Arena arena = arena_init(PROT_READ | PROT_WRITE);
Vector v = vector_init(arena_allocator(&arena), sizeof(int));
vector_push_t(&v, int, 10);
vector_push_t(&v, int, 20);
```

Note that since arenas do not support realloc or free, so growth always allocates new blocks and old memory stays alive in the arena.

### API Overview
```c
// Initialization
Vector vector_init(Allocator alloc, size_t elem_size);
#define vector_init_malloc(T)

// Capacity
void vector_reserve(Vector *v, size_t capacity);
void vector_shrink_to_fit(Vector *v);

// Modifiers
void vector_push(Vector *v, void *elem);
void vector_insert(Vector *v, size_t index, void *elem);
void vector_pop(Vector *v);
void vector_erase(Vector *v, size_t index);
void vector_clear(Vector *v);
void vector_destroy(Vector *v);

// Typed helper macros
vector_push_t(&v, int, 123);
vector_insert_t(&v, int, 2, 42);

int x = vector_at(&v, int, 0);
int *data = vector_data_as(&v, int);
int y = vector_front(&v, int);
int z = vector_back(&v, int);
```

### Safety Notes

This vector does not perform bounds checking except for debug asserts. All pointer access assumes correct usage.
