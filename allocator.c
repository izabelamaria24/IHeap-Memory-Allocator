#include "allocator.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <pthread.h>

static IHeap heap = {0};
static int heap_capacity = 1024;

static pthread_mutex_t heap_mutex = PTHREAD_MUTEX_INITIALIZER; // TODO
static pthread_cond_t coalesce_cond = PTHREAD_COND_INITIALIZER; // TODO

static pthread_t gc_thread; // for garbage collection // TODO
static volatile int running = 1;

size_t align(size_t size)
{
  return (size + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1);
}

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

  pthread_create(&gc_thread, NULL, garbage_collector, NULL);
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

  pthread_mutex_lock(&heap_mutex);

  if (heap.head == NULL) 
  {
    iheap_init(&heap, heap_capacity);
  }

  heap_capacity -= (BLOCK_SIZE + size);
  
  size_t total_size_requested = align(size) + BLOCK_SIZE;
  
  IHeap_Chunk* best_fit_chunk = find_best_fit_chunk(&heap, total_size_requested);
  if (best_fit_chunk == NULL) 
  {
    pthread_mutex_unlock(&heap_mutex);
    return NULL;
  }

  split_chunk(best_fit_chunk, total_size_requested);
  
  pthread_mutex_unlock(&heap_mutex);
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

  pthread_mutex_lock(&heap_mutex);
  
  IHeap_Chunk* chunk = (IHeap_Chunk*)((char*)ptr - BLOCK_SIZE); 

  chunk->allocated = false;

  pthread_cond_signal(&coalesce_cond);

  pthread_mutex_unlock(&heap_mutex);
}

void* garbage_collector(void* arg)
{
  while (running)
  {
    pthread_mutex_lock(&heap_mutex);

    pthread_cond_wait(&coalesce_cond, &heap_mutex);

    IHeap_Chunk* current_chunk = heap.head;
    while (current_chunk != NULL)
    {
      if (!current_chunk->allocated)
      {
        coalesce_chunks(current_chunk);
      }
      current_chunk = current_chunk->next_chunk;
    }

    pthread_mutex_unlock(&heap_mutex);
  }

  return NULL;
}

void iheap_cleanup()
{
  running = 0;
  pthread_cond_signal(&coalesce_cond);
  pthread_join(gc_thread, NULL);
}

void cleanup_on_exit()
{
  iheap_cleanup();
}

__attribute__((constructor)) void register_cleanup() {
    atexit(cleanup_on_exit);
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
  return 0;
}
