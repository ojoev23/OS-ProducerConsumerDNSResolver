#include "multi-lookup.h"
#include "array.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <pthread.h>

pthread_mutex_t log_mutex;
FILE* log_file_ptr;
array s;

static int resolve_ipv4_address(const char *hostname, char *ipv4, size_t ipv4_len) {
  struct addrinfo hints;
  struct addrinfo *results = NULL;
  struct addrinfo *current = NULL;
  int status = 0;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  status = getaddrinfo(hostname, NULL, &hints, &results);
  if (status != 0) {
    return -1;
  }

  for (current = results; current != NULL; current = current->ai_next) {
    struct sockaddr_in *address = (struct sockaddr_in *) current->ai_addr;
    if (inet_ntop(AF_INET, &(address->sin_addr), ipv4, ipv4_len) != NULL) {
      freeaddrinfo(results);
      return 0;
    }
  }

  freeaddrinfo(results);
  return -1;
}

void* requester_thread(void* arg){
  char* filename = (char*) arg;
  FILE* input_file = fopen(filename, "r");
  if (!input_file) {
    fprintf(stderr, "Invalid file %s\n", filename);
    return NULL;
  }
  char hostname[1025];
  while(fgets(hostname, sizeof(hostname), input_file)){
    hostname[strcspn(hostname, "\n")] = 0;
    array_put(&s, strdup(hostname));
  }
  fclose(input_file);
  return NULL;
}

void* resolver_thread(void* arg){
  (void)arg;
  char ipv4[INET_ADDRSTRLEN];
  while(1){
    char* hostname;
    array_get(&s, &hostname);
    if(hostname == NULL){
      break;
    }
    int status = resolve_ipv4_address(hostname, ipv4, INET_ADDRSTRLEN);
    pthread_mutex_lock(&log_mutex);
    if(status == -1){
      fprintf(log_file_ptr, "%s -> NOT_RESOLVED\n", hostname);
    }
    else{
      fprintf(log_file_ptr, "%s -> %s\n", hostname, ipv4);
    }
    pthread_mutex_unlock(&log_mutex);
    free(hostname);
  }
  return NULL;
}

int main(int argc, char *argv[]) {
  if(argc < 3) {
    fprintf(stderr, "Not enough arguments");
    return 1;
  }

  struct timeval start, end;
  gettimeofday(&start, NULL);
  array_init(&s);
  pthread_mutex_init(&log_mutex, NULL);
  log_file_ptr = fopen(argv[1], "w");
  if(!log_file_ptr){
    fprintf(stderr, "Log file did not open\n");
    return 1;
  }

  int input_files = argc - 2;
  int num_resolvers = 8;
  pthread_t requesters[input_files];
  pthread_t resolvers[num_resolvers];

  for(int i = 0; i < num_resolvers; i++){
    pthread_create(&resolvers[i], NULL, resolver_thread, NULL);
  }
  for(int i = 0; i < input_files; i++){
    pthread_create(&requesters[i], NULL, requester_thread, argv[i+2]);
  }
  for(int i = 0; i < input_files; i++){
    pthread_join(requesters[i], NULL);
  }
  
  for(int i = 0; i < num_resolvers; i++){
    array_put(&s, NULL);
  }
  for(int i = 0; i < num_resolvers; i++){
    pthread_join(resolvers[i], NULL);
  }
  pthread_mutex_destroy(&log_mutex);
  fclose(log_file_ptr);
  array_free(&s);
  gettimeofday(&end, NULL);
  double elapsed = (end.tv_sec - start.tv_sec) + ((end.tv_usec - start.tv_usec) / 1000000.0);
  printf("./multi-lookup: total time is %f seconds\n", elapsed);
  return 0;
}
