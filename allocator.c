#include "allocator.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>

static IHeap heap = {0};
int heap_capacity = 1024;

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

IHeap_Chunk* find_best_fit_chunk(IHeap* heap, size_t total_size_requested)
{
  IHeap_Chunk* current_chunk = (IHeap_Chunk*)heap->head;
  IHeap_Chunk* best_fit_chunk = NULL;

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
    size_t remaining_chunk_size = best_fit_chunk->chunk_size - total_size_requested;

    if (remaining_chunk_size > BLOCK_SIZE)
    {
        IHeap_Chunk* new_chunk = (IHeap_Chunk*)((char*)best_fit_chunk + total_size_requested);

        new_chunk->chunk_size = remaining_chunk_size;
        new_chunk->allocated = false;
        new_chunk->prev_chunk = best_fit_chunk;
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
  
  return (void*)((char*)best_fit_chunk + BLOCK_SIZE);
}

void coalesce_chunks(IHeap_Chunk* ptr)
{
  if (ptr == NULL) return;

  bool prev_state = (ptr->prev_chunk != NULL && !ptr->prev_chunk->allocated);
  bool next_state = (ptr->next_chunk != NULL && !ptr->next_chunk->allocated);

  // Case 1: both prev and next chunks are allocated or are NULL (do nothing)
  if (!prev_state && !next_state) return;

  // Case 2: both prev and next chunks are free and not NULL
  if (prev_state && next_state)
  {
    int total_available_size = 2 * BLOCK_SIZE + ptr->prev_chunk->chunk_size + ptr->chunk_size + ptr->next_chunk->chunk_size;
    ptr->prev_chunk->next_chunk = ptr->next_chunk->next_chunk;
    
    if (ptr->next_chunk->next_chunk)
    {
      ptr->next_chunk->next_chunk->prev_chunk = ptr->prev_chunk;
    }

    ptr->prev_chunk->chunk_size = total_available_size;
    ptr->prev_chunk->allocated = false;
    ptr = ptr->prev_chunk;
  } 
  // Case 3: only the previous chunk is free
  else if (prev_state)
  {
    int total_available_size = BLOCK_SIZE + ptr->prev_chunk->chunk_size + ptr->chunk_size;
    ptr->prev_chunk->next_chunk = ptr->next_chunk;

    if (ptr->next_chunk)
    {
      ptr->next_chunk->prev_chunk = ptr->prev_chunk;
    }

    ptr->prev_chunk->chunk_size = total_available_size;
    ptr->prev_chunk->allocated = false;
    ptr = ptr->prev_chunk;
  }
  // Case 4: only the next chunk is free
  else if (next_state)
  {
    int total_available_size = BLOCK_SIZE + ptr->chunk_size + ptr->next_chunk->chunk_size;
    ptr->next_chunk = ptr->next_chunk->next_chunk;

    if (ptr->next_chunk)
    {
      ptr->next_chunk->prev_chunk = ptr;
    }

    ptr->chunk_size = total_available_size;
    ptr->allocated = false;
  }
}

void iheap_free(void* ptr)
{
  if (ptr == NULL) return;
  
  IHeap_Chunk* chunk = (IHeap_Chunk*)((char*)ptr - BLOCK_SIZE); 

  chunk->allocated = false;

  coalesce_chunks(chunk);
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
    printf("=== Initializing Heap ===\n");
    print_heap(&heap);

    void* blocks[10] = {NULL};
    int block_count = 0; 
    printf("block size: %d\n", BLOCK_SIZE);
    printf("=== Allocating 24 bytes ===\n");
    blocks[block_count++] = iheap_malloc(24);
    print_heap(&heap);
    printf("Allocated pointers: ");
    for (int i = 0; i < block_count; i++) {
        printf("%p ", blocks[i]);
    }
    printf("\n");

    printf("=== Allocating 64 bytes ===\n");
    blocks[block_count++] = iheap_malloc(64);
    print_heap(&heap);
    printf("Allocated pointers: ");
    for (int i = 0; i < block_count; i++) {
        printf("%p ", blocks[i]);
    }
    printf("\n");

    printf("=== Allocating 128 bytes ===\n");
    blocks[block_count++] = iheap_malloc(128);
    print_heap(&heap);
    printf("Allocated pointers: ");
    for (int i = 0; i < block_count; i++) {
        printf("%p ", blocks[i]);
    }
    printf("\n");

    printf("=== Freeing block of 64 bytes ===\n");
    iheap_free(blocks[1]);
    block_count--;  
    print_heap(&heap);
    printf("Allocated pointers: ");
    for (int i = 0; i < block_count; i++) {
        printf("%p ", blocks[i]);
    }
    printf("\n");

    printf("=== Freeing block of 24 bytes ===\n");
    iheap_free(blocks[0]);
    block_count--;  
    print_heap(&heap);
    printf("Allocated pointers: ");
    for (int i = 0; i < block_count; i++) {
        printf("%p ", blocks[i]);
    }
    printf("\n");

    printf("=== Freeing block of 128 bytes ===\n");
    iheap_free(blocks[2]);
    block_count--; 
    print_heap(&heap);
    printf("Allocated pointers: ");
    for (int i = 0; i < block_count; i++) {
        printf("%p ", blocks[i]);
    }
    printf("\n");

    printf("=== Allocating 200 bytes (should fit if coalescing succeeded) ===\n");
    blocks[block_count++] = iheap_malloc(200);
    print_heap(&heap);
    printf("Allocated pointers: ");
    for (int i = 0; i < block_count; i++) {
        printf("%p ", blocks[i]);
    }
    printf("\n");

    printf("=== Attempting to allocate 2000 bytes (should fail) ===\n");
    void* block5 = iheap_malloc(2000);
    if (block5 == NULL) {
        printf("Allocation of 2000 bytes failed as expected.\n");
    }

    printf("=== Freeing and Cleaning Up ===\n");
    if (block_count > 0) {
        iheap_free(blocks[block_count - 1]);  
        block_count--; 
    }
    print_heap(&heap);

    return 0;
}
