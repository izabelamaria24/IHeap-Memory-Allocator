#include "allocator.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>

static IHeap heap = {0};
int heap_capacity = 1024;

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

// find best fit chunk method
IHeap_Chunk* find_best_fit_chunk(IHeap* heap, size_t total_size_requested)
{
  IHeap_Chunk* current_chunk = heap->head;
  IHeap_Chunk* best_fit_chunk = NULL;


  printf("HEAD CHUNK SIZE: %d\n", heap->head->chunk_size);

  while (current_chunk) {
     if (!current_chunk->allocated && current_chunk->chunk_size >= total_size_requested) 
    {
      if (best_fit_chunk == NULL || best_fit_chunk->chunk_size > current_chunk->chunk_size)  {
          best_fit_chunk = current_chunk;
      }
    }
    current_chunk = current_chunk->next_chunk;
  }

  return best_fit_chunk;
}

void split_chunk(IHeap_Chunk* best_fit_chunk, size_t total_size_requested)
{
    printf("Found chunk size: %d\n", best_fit_chunk->chunk_size);
    size_t remaining_chunk_size = best_fit_chunk->chunk_size - total_size_requested;

    if (remaining_chunk_size > BLOCK_SIZE)
    {
        IHeap_Chunk* new_chunk = (IHeap_Chunk*)((char*)best_fit_chunk + total_size_requested);

        new_chunk->chunk_size = remaining_chunk_size;
        new_chunk->allocated = false;
        new_chunk->next_chunk = best_fit_chunk->next_chunk;
        best_fit_chunk->next_chunk = new_chunk;
        best_fit_chunk->chunk_size = total_size_requested - BLOCK_SIZE;
    }

    best_fit_chunk->allocated = true;
}

void* iheap_malloc(size_t size)
{
  assert(size > 0);

  if (heap.head == NULL) 
  {
    iheap_init(&heap, heap_capacity);
  }

  heap_capacity -= (BLOCK_SIZE + size);
  
  size_t total_size_requested = size + BLOCK_SIZE;
  
  IHeap_Chunk* best_fit_chunk = find_best_fit_chunk(&heap, total_size_requested);
  if (best_fit_chunk == NULL) return NULL;

  split_chunk(best_fit_chunk, total_size_requested);

  return best_fit_chunk->data;
}

void iheap_free(void* ptr)
{
  // TODO
}

void print_heap(IHeap* heap) {
    IHeap_Chunk* current = heap->head;
    printf("\nHeap Structure:\n");
    printf("------------------------------------------------\n");
    printf("| Address       | Size       | Allocated | Next |\n");
    printf("------------------------------------------------\n");
    
    while (current != NULL) {
        printf("| %p | %-10zu | %-9s | %p |\n", 
               (void*)current, 
               current->chunk_size, 
               current->allocated ? "Yes" : "No", 
               (void*)current->next_chunk);
        
        current = current->next_chunk;
    }
    printf("------------------------------------------------\n\n");
}

int main()
{ 
  print_heap(&heap);
    printf("Block size: %ld\n", BLOCK_SIZE);
    printf("Heap capacity: %ld\n", heap_capacity);

    void* block1 = iheap_malloc(24);
    printf("Head chunk size: %ld\n", heap.head->chunk_size);
    print_heap(&heap);  
  
     printf("Block size: %ld\n", BLOCK_SIZE);

    printf("Heap capacity: %ld\n", heap_capacity);
    void* block2 = iheap_malloc(64);
    print_heap(&heap);  
    
 printf("Block size: %ld\n", BLOCK_SIZE);

    printf("Heap capacity: %ld\n", heap_capacity);
    void* block3 = iheap_malloc(128);
    print_heap(&heap);  

    return 0;
}
