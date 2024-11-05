#ifndef IHEAP_ALLOCATOR_H
#define IHEAP_ALLOCATOR_H

#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>

typedef struct IHeap_Chunk
{
  struct IHeap_Chunk* prev_chunk;
  struct IHeap_Chunk* next_chunk;
  size_t chunk_size;
  bool allocated;
  char data[];
} IHeap_Chunk;

typedef struct IHeap
{
  IHeap_Chunk* head;
  size_t heap_capacity;
} IHeap; 

#define BLOCK_SIZE sizeof(IHeap_Chunk)
#define ALIGNMENT 4

void* iheap_malloc(size_t size);
void iheap_free(void* ptr);
void iheap_init(IHeap* heap, size_t heap_capacity);

#endif // IHEAP_ALLOCATOR_H
