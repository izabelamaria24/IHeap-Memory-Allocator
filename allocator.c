#include "allocator.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>

static IHeap heap = {0};

// initialize the heap
void iheap_init(IHeap* heap, size_t heap_capacity)
{
  heap->head = (IHeap_Chunk*)sbrk(heap_capacity + BLOCK_SIZE);

  if (heap->head == (void*)-1)
  {
    perror("Failed to allocate heap!");
    return;
  }

  heap->head->prev_chunk = NULL;
  heap->head->next_chunk = NULL;
  heap->head->chunk_size = heap_capacity;
  heap->head->allocated = false;
  heap->heap_capacity = heap_capacity;
}

void* iheap_malloc(size_t size)
{
  assert(size > 0);

  if (heap.head == NULL) 
  {
    iheap_init(&heap, 640000);
  }
  
  size_t total_size_requested = size + BLOCK_SIZE;
  IHeap_Chunk* current_chunk = heap.head;
  IHeap_Chunk* best_fit_chunk = NULL;

  while (current_chunk)
  {
    if (!current_chunk->allocated && current_chunk->chunk_size >= total_size_requested)
    {
      if (best_fit_chunk == NULL || best_fit_chunk->chunk_size > current_chunk->chunk_size)
      {
        best_fit_chunk = current_chunk;
      }
    }

    current_chunk = current_chunk->next_chunk;
  }

  if (best_fit_chunk == NULL) return NULL;

  // check if the found chunk is larger than needed
  if (best_fit_chunk->chunk_size > total_size_requested)
  {
    // TODO chunk split
  }

  return NULL;
}

void iheap_free(void* ptr)
{
  // TODO
}

int main()
{
  printf("Allocated block: %p\n", iheap_malloc(24));
}
