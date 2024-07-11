#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int main(void) {
  // define address and port
  const char *ADDRESS = "127.0.0.1";
  int PORT = 6000;
  // declare client and server
  struct sockaddr_in client, server;
  // declare file descriptors for the socket and connection
  int socket_file_descriptor, connection_file_descriptor;
  unsigned int n;
  char receiver_buffer[100] = "",
       sender_buffer[100] =
           "HTTP/1.1 200 OK\r\nContent-Length: 13\r\n\r\nHello, world!";
  // create tcp socket
  socket_file_descriptor = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_file_descriptor == -1) {
    perror("Error creating socket\n");
    return 1;
  }
  // initialize server
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
    printf("Error listening on socket\n");
    close(socket_file_descriptor);
    return 1;
  }
  // accept client connections
  n = sizeof client;
  connection_file_descriptor =
      accept(socket_file_descriptor, (struct sockaddr *)&client, &n);
  if (connection_file_descriptor == -1) {
    perror("Error accepting connection\n");
    close(socket_file_descriptor);
    return 1;
  }
  // receive messages from the client
  recv(connection_file_descriptor, receiver_buffer, sizeof receiver_buffer, 0);
  printf("\n[client] %s", receiver_buffer);
  printf("\nserver: %s", sender_buffer);
  send(connection_file_descriptor, sender_buffer, sizeof sender_buffer, 0);
  close(connection_file_descriptor);
  close(socket_file_descriptor);
  return 0;
}
