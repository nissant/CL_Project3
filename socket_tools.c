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

#define HTTP_SEPARATORE_SIZE 4
const char *HTTP_SEPARATORE = "\r\n\r\n";
const char *SIGNAL_INVALID_REQUEST = "Invalid client request \r \r\n\r\n";

unsigned int getRandomPortInRange_TestFail()
{
  if (random() % 2) {
    return (unsigned int)FAIL_PORT;
  } else {
    return (unsigned int)((random() % (UPPER_PORT - LOWER_PORT + 1)) + LOWER_PORT);
  }
}

void createBindSocket_LogPort(int *socket_ptr, FILE *file_log)
{
  int new_socket;
  int error_code;
  unsigned int port;
  struct sockaddr_in service;
  new_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (new_socket == -1) {
    printf("socket creation failed...\n");
  }
  bzero(&service, sizeof(service));

  service.sin_family = AF_INET;
  service.sin_addr.s_addr = INADDR_ANY;
  port = getRandomPortInRange_TestFail();
  service.sin_port = htons(port);

  while ((bind(new_socket, (struct sockaddr *)&service, sizeof(service))) != 0) {
    error_code = errno;

    if (EADDRINUSE == error_code || EINVAL == error_code || EACCES == error_code) {
      port = getRandomPortInRange_TestFail();
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

  server_port_fp = fopen("server_port", "w");
  createBindSocket_LogPort(&sockfd_server, server_port_fp);
  http_port_fp = fopen("http_port", "w");
  createBindSocket_LogPort(&sockfd_client, http_port_fp);

  if ((listen(sockfd_client, LISTEN_COUNT)) != 0 || (listen(sockfd_server, LISTEN_COUNT)) != 0) {
    printf("Listen failed...\n");
    exit(-1);
  }

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

void closeSession(int socket)
{
  if (close(socket) != 0) {
    printf("close(socket) failed, error %d\n", errno);
    exit(-1);
  }
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

int checkInvalidRequest(char *inBuffer, unsigned int msg_size)
{
  char *string_end_pntr = memchr(inBuffer, '\0', msg_size);
  if (string_end_pntr < (char *)(inBuffer + msg_size) && string_end_pntr != NULL) {
    strcpy(inBuffer, SIGNAL_INVALID_REQUEST);
    return 1;
  } else {
    return 0;
  }
}

int send_msg(int connfd_client, char *buffer)
{
  char *CurPlacePtr = buffer;
  int BytesTransferred;
  int RemainingBytesToSend = strlen(buffer);  // Sent message structure is known and in my control, thus strlen is fine

  while (RemainingBytesToSend > 0) {
    BytesTransferred = send(connfd_client, CurPlacePtr, RemainingBytesToSend, 0);
    if (BytesTransferred <= 0) {
      printf("send() failed, error %d\n", errno);
      exit(-1);
    }

    RemainingBytesToSend -= BytesTransferred;
    CurPlacePtr += BytesTransferred;
  }
  return 0;
}

int recv_msg(int socket, unsigned int msgSeparatorCount, char **msgBuffer, unsigned int *bufferSize,
             unsigned int *msgSize)
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
    CurPlacePtr = CurPlacePtr + BytesJustTransferred;
    TotalBytesTransferred += BytesJustTransferred;
    TotalAvailableBuffer -= BytesJustTransferred;

    if (memSeparatoreCount(*msgBuffer, TotalBytesTransferred) == msgSeparatorCount) {
      *msgSize = TotalBytesTransferred;
      return 0;
    }

    if (TotalAvailableBuffer < BUFFER_INIT_SIZE) {
      *msgBuffer = (char *)realloc(*msgBuffer, sizeof(char) * (*bufferSize + BUFFER_INIT_SIZE));
      if (*msgBuffer == NULL) {
        printf("Realloc of msg buffer failed! Can't complete the task.");
        exit(-1);
      }
      TotalAvailableBuffer += BUFFER_INIT_SIZE;
      *bufferSize += BUFFER_INIT_SIZE;
    }
  }
  // recv() returns zero if connection was gracefully disconnected.
  return -1;
}
