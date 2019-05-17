#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "socket_tools.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define LISTEN_COUNT 10
#define FAIL_PORT 80
#define LOWER_PORT 1024
#define UPPER_PORT 64000
#define SA struct sockaddr
#define HTTP_SEPARATORE_SIZE 4

const char *HTTP_SEPARATORE = "\r\n\r\n";

unsigned int getRandomPortInRange()
{  // Generates a random port number in range [lower, upper].
  if (random() % 2) {
    return (unsigned int)FAIL_PORT;
  } else {
    return (unsigned int)((random() % (UPPER_PORT - LOWER_PORT + 1)) + LOWER_PORT);
  }
}

void createBindSocket_LogPort(int *socket_ptr, FILE *file_log)
{
  // socket create and verification
  int new_socket;
  int error_code;
  unsigned int port;
  struct sockaddr_in service;
  new_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (new_socket == -1) {
    printf("socket creation failed...\n");
  }
  bzero(&service, sizeof(service));
  // assign IP, Port will fail half of the calls this will test the random port feature
  service.sin_family = AF_INET;
  service.sin_addr.s_addr = INADDR_ANY;
  port = getRandomPortInRange();
  service.sin_port = htons(port);

  // Binding newly created socket to given IP and verification
  while ((bind(new_socket, (SA *)&service, sizeof(service))) != 0) {
    error_code = errno;

    if (EADDRINUSE == error_code || EINVAL == error_code || EACCES == error_code) {

      port = getRandomPortInRange();
      service.sin_port = htons(port);
    } else {
      printf("socket bind failed...\n");
    }
  }
  fprintf(file_log, "%u\n", port);
  fclose(file_log);
  *socket_ptr = new_socket;
  return;
}

void connectServersClient()
{
  FILE *server_port_fp;
  FILE *http_port_fp;
  // Create client/server sockets
  server_port_fp = fopen("server_port", "w");
  createBindSocket_LogPort(&sockfd_server, server_port_fp);
  http_port_fp = fopen("http_port", "w");
  createBindSocket_LogPort(&sockfd_client, http_port_fp);

  // Now server is ready to listen and verification
  if ((listen(sockfd_client, LISTEN_COUNT)) != 0 || (listen(sockfd_server, LISTEN_COUNT)) != 0) {
    printf("Listen failed...\n");
    exit(-1);
  }

  // Accept the data packet from client and verification
  int i;
  for (i = 0; i < SERVER_COUNT; i++) {
    connfd_server[i] = acceptSession(sockfd_server);
  }
  connfd_client = acceptSession(sockfd_client);
}

int acceptSession(int sokcet)
{
  int connection = accept(sokcet, NULL, NULL);
  if (connfd_client < 0) {
    printf("Client acccept failed...\n");
    exit(-1);
  }
  return connection;
}

unsigned int memSeparatoreCount(char *inBuffer, unsigned int dataSize)
{
  unsigned int count = 0;
  char *buffer_pntr = inBuffer;
  unsigned int i, j, matchFlag;

  for (i = 0; i < dataSize - HTTP_SEPARATORE_SIZE + 1; i++) {
    matchFlag = 1;
    for (j = 0; j < HTTP_SEPARATORE_SIZE; j++) {
      if (buffer_pntr[i + j] != HTTP_SEPARATORE[j]) {
        matchFlag = 0;
        break;
      }
    }
    count += (matchFlag == 1 ? 1 : 0);
  }
  return count;
}

int send_msg(int connfd_client, char *buffer)
{
  char *CurPlacePtr = buffer;
  int BytesTransferred;
  int RemainingBytesToSend = strlen(buffer);  // Sent message structure is known and in my control, thus strlen is fine

  while (RemainingBytesToSend > 0) {
    // send does not guarantee that the entire message is sent
    BytesTransferred = send(connfd_client, CurPlacePtr, RemainingBytesToSend, 0);
    if (BytesTransferred <= 0) {
      printf("send() failed, error %d\n", errno);
      exit(-1);
    }

    RemainingBytesToSend -= BytesTransferred;
    CurPlacePtr += BytesTransferred;  // pointer arithmetic
  }

  return 0;
}

int recv_msg(int socket, enum msg_type msgType, char **msgBuffer, unsigned int *bufferSize)
{
  char *CurPlacePtr = *msgBuffer;
  int BytesJustTransferred = 0;
  unsigned int TotalBytesTransferred = 0, TotalAvailableBuffer = *bufferSize;
  bzero(CurPlacePtr, TotalAvailableBuffer);

  while ((BytesJustTransferred = recv(socket, CurPlacePtr, TotalAvailableBuffer, 0)) > 0) {
    if (BytesJustTransferred < 0) {
      printf("recv() failed, error %d\n", errno);
      exit(-1);
    }
    CurPlacePtr = CurPlacePtr + BytesJustTransferred;  // pointer arithmetic
    TotalBytesTransferred += BytesJustTransferred;
    TotalAvailableBuffer -= BytesJustTransferred;

    if (memSeparatoreCount(*msgBuffer, TotalBytesTransferred) == msgType) {
      // MSG Complete - end the session
      close(socket);
      return 0;
    }

    // MSG Not Complete, allocate more buffer space to make sure no data is lost in next transaction
    *msgBuffer = (char *)realloc(*msgBuffer, sizeof(char) * (*bufferSize + BUFFER_INIT_SIZE));
    if (*msgBuffer == NULL) {
      printf("Realloc of msg buffer failed! Can't complete the task.");
      exit(-1);
    }
    TotalAvailableBuffer += BUFFER_INIT_SIZE;
    *bufferSize += BUFFER_INIT_SIZE;
  }
  printf("Returned -1 from recv()\n");
  exit(-1);
  // return -1; // recv() returns zero if connection was gracefully disconnected.
}
