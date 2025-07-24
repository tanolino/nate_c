/*

nate_DynMemory

Do:
    #define NATE_DYNMEMORY_IMPLEMENTATION
before you include this header in only one c / c++ compilation unit

CC0 License

by Nathanael Krauße

*/

#ifndef _NATE_DYNMEMORY_HEADER
#define _NATE_DYNMEMORY_HEADER
#include <stdlib.h>

typedef struct ByteBufferTag {
    char* data;
    size_t size;
    size_t allocated;
} ByteBuffer;
#define ByteBuffer0 (ByteBuffer){0};

// Reuses allocated memory if sufficient, returns 1 on success and 0 on failure
extern int nate_ByteBuffer_Alloc(ByteBuffer* buffer, size_t size);
extern void nate_ByteBuffer_Free(ByteBuffer* buffer);

typedef struct ArrayBufferTag {
    void* data;
    size_t size;
    size_t allocated;
} ArrayBuffer;
#define ArrayBuffer0 (ArrayBuffer){0};

// Reuses allocated memory if sufficient, returns 1 on succes and 0 on failure
extern int nate_ArrayBuffer_Alloc(ArrayBuffer* buffer, size_t elem_size, size_t size);
extern void nate_ArrayBuffer_Free(ArrayBuffer* buffer);

typedef struct MemoryOf3rdTag {
    void* data;
    void (*free_fn)(void*);
} MemoryOf3rd;
#define MemoryOf3rd0 (MemoryOf3rd){0};

extern void nate_MemoryOf3rd_Free(MemoryOf3rd* memory);

//------------------- Impl

#ifdef NATE_DYNMEMORY_IMPLEMENTATION

int nate_ByteBuffer_Alloc(ByteBuffer *buffer, size_t size)
{
    if (!buffer) return 0;
    // We have enough
    if (buffer->allocated >= size) {
        return 1;
    }
    if (!size) {
        nate_ByteBuffer_Free(buffer);
        return 1;
    }
    
    void* new_mem = realloc(buffer->data, sizeof(char) * size);
    if (!new_mem)
        return 0;
    buffer->data = new_mem;
    buffer->allocated = size;
    return 1;
}

void nate_ByteBuffer_Free(ByteBuffer* buffer)
{
    if (!buffer) return;
    if (!buffer->data) return;
    free(buffer->data);
    buffer->data = NULL;
    buffer->allocated = 0;
    buffer->size = 0;
}

int nate_ArrayBuffer_Alloc(ArrayBuffer* buffer, size_t elem_size, size_t size)
{
    if (!buffer) return 0;
    if (buffer->allocated >= size) {
        return 1;
    }
    if (!size) {
        nate_ArrayBuffer_Free(buffer);
        return 1;
    }
    
    void* new_mem = realloc(buffer->data, elem_size * size);
    if (!new_mem)
        return 0;
    buffer->data = new_mem;
    buffer->allocated = size;
    return 1;
}

void nate_ArrayBuffer_Free(ArrayBuffer* buffer)
{
    if (!buffer) return;
    if (!buffer->data) return;
    free(buffer->data);
    buffer->data = NULL;
    buffer->allocated = 0;
    buffer->size = 0;
}

void nate_MemoryOf3rd_Free(MemoryOf3rd* memory)
{
    if (!memory) return;
    if (!memory->data) return;
    if (!memory->free_fn) {
        // try our best
        free(memory->data);
        memory->data = NULL;
        return;
    }
    else {
        memory->free_fn(memory->data);
        memory->data = NULL;
        memory->free_fn = NULL;
    }
}

#endif // NATE_DYNMEMORY_IMPLEMENTATION

#endif // _NATE_DYNMEMORY_HEADER
