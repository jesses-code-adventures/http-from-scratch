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

void *handle_client(void *arg) {
  client_info_t *client_info = (client_info_t *)arg;
  int connection_file_descriptor = client_info->connection_file_descriptor;
  char receiver_buffer[BUFFER_SIZE] = "",
       sender_buffer[BUFFER_SIZE] =
           "HTTP/1.1 200 OK\r\nContent-Length: 13\r\n\r\nHello, world!";
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
  client_info_t client_info[MAX_THREADS];
  pthread_t thread[MAX_THREADS];
  int thread_count = 0;

  while (1) {
    n = sizeof client_info[thread_count].client_addr;
    client_info[thread_count].connection_file_descriptor =
        accept(socket_file_descriptor, (struct sockaddr *)&client, &n);
    if (client_info[thread_count].connection_file_descriptor == -1) {
      perror("Error creating connection\n");
      continue;
    }
    if (pthread_create(&thread[thread_count], NULL, handle_client,
                       &client_info[thread_count]) != 0) {
      perror("Error creating thread\n");
      close(client_info[thread_count].connection_file_descriptor);
      continue;
    }

    pthread_detach(thread[thread_count]);
    thread_count = (thread_count + 1) % MAX_THREADS;
  }
  close(socket_file_descriptor);
  return 0;
}
