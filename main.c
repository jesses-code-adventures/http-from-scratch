#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define PORT 6000
#define BUFFER_SIZE 1024
#define MAX_THREADS 100
#define ADDRESS "127.0.0.1"

typedef struct {
  int connection_file_descriptor;
  struct sockaddr_in client_addr;
} client_info_t;

typedef struct {
  int status_code;
  size_t content_length;
  char *content_type;
  char *content;
} http_response_t;

void create_response(int status_code, char *content_type, char *content,
                     http_response_t *responses, int thread_index) {
  if (content == NULL) {
    return;
  }
  size_t content_length = strlen(content);
  responses[thread_index].content_length = content_length;
  responses[thread_index].status_code = status_code;
  responses[thread_index].content_type = content_type;
  responses[thread_index].content = content;
  return;
}

char *buffer_ok_response(http_response_t *response, char *buffer) {
  sprintf(buffer,
          "HTTP/1.1 %d OK\r\nContent-Length: %ld\r\nContent-Type: %s\r\n\r\n%s",
          response->status_code, response->content_length,
          response->content_type, response->content);
  return buffer;
}

typedef struct {
  client_info_t *client_info;
  http_response_t *responses;
  int thread_index;
} handle_client_args_t;

void *handle_client(void *arg) {
  handle_client_args_t *args = (handle_client_args_t *)arg;
  int connection_file_descriptor =
      args->client_info->connection_file_descriptor;
  char receiver_buffer[BUFFER_SIZE] = "", sender_buffer[BUFFER_SIZE] = "";
  create_response(200, "text/html", "Hello, World!", args->responses,
                  args->thread_index);
  buffer_ok_response(&args->responses[args->thread_index], sender_buffer);
  // receive messages from the client
  recv(connection_file_descriptor, receiver_buffer, sizeof receiver_buffer, 0);
  printf("\n[client] %s", receiver_buffer);
  printf("\nserver: %s", sender_buffer);
  send(connection_file_descriptor, sender_buffer, sizeof sender_buffer, 0);
  close(connection_file_descriptor);
  return NULL;
}

int init_server(struct sockaddr_in server, int socket_file_descriptor) {
  memset(&server, 0, sizeof(server));
  server.sin_family = AF_INET;
  server.sin_port = htons(PORT);
  server.sin_addr.s_addr = inet_addr(ADDRESS);
  // bind the socket to the server address
  if (bind(socket_file_descriptor, (struct sockaddr *)&server,
           sizeof(server)) == -1) {
    perror("Error binding socket");
    close(socket_file_descriptor);
    return 1;
  }
  // listen for connections
  if (listen(socket_file_descriptor, 1) == -1) {
    perror("Error listening on socket\n");
    close(socket_file_descriptor);
    return 1;
  }
  return 0;
}

int main(void) {
  // declare client and server
  struct sockaddr_in client, server;
  // declare file descriptors for the socket and connection
  int socket_file_descriptor, connection_file_descriptor;
  unsigned int n;
  // create tcp socket
  socket_file_descriptor = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_file_descriptor == -1) {
    perror("Error creating socket\n");
    return 1;
  }
  init_server(server, socket_file_descriptor);
  handle_client_args_t args[MAX_THREADS];
  client_info_t clients[MAX_THREADS];
  http_response_t responses[MAX_THREADS];
  pthread_t threads[MAX_THREADS];
  int thread_count = 0;
  while (1) {
    n = sizeof clients[thread_count].client_addr;
    clients[thread_count].connection_file_descriptor =
        accept(socket_file_descriptor, (struct sockaddr *)&client, &n);
    if (clients[thread_count].connection_file_descriptor == -1) {
      perror("Error creating connection file descriptor\n");
      continue;
    }
    args[thread_count].client_info = &clients[thread_count];
    args[thread_count].responses = responses;
    args[thread_count].thread_index = thread_count;
    if (pthread_create(&threads[thread_count], NULL, handle_client,
                       &args[thread_count]) != 0) {
      perror("Error creating thread\n");
      close(clients[thread_count].connection_file_descriptor);
      continue;
    }
    pthread_detach(threads[thread_count]);
    thread_count = (thread_count + 1) % MAX_THREADS;
  }
  close(socket_file_descriptor);
  return 0;
}
