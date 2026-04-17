#include "array.h"

// your thread-safe array implementation here

int array_init(array* s) { 
  sem_init(&s->mutex, 0, 1);
  sem_init(&s->full, 0, 0);
  sem_init(&s->empty, 0, ARRAY_SIZE);
  s->top = 0;
  return 0;
}

int array_put(array* s, char* hostname) {
  sem_wait(&s->empty);
  sem_wait(&s->mutex);
  
  s->arr[s->top] = hostname;
  s->top++;
  
  sem_post(&s->mutex);
  sem_post(&s->full);
  return 0;
}

int array_get(array* s, char** hostname) {
  sem_wait(&s->full);
  sem_wait(&s->mutex); 

  s->top--;
  *hostname = s->arr[s->top];
  
  sem_post(&s->mutex);
  sem_post(&s->empty);
  return 0;
}

void array_free(array* s) {
  sem_destroy(&s->mutex);
  sem_destroy(&s->full);
  sem_destroy(&s->empty);
}

