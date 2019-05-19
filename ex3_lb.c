#include "socket_tools.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define NO_BODY_REQUEST 1  // HTTP request with no body part (GET) - 1 separator
#define RESPONSE 2         // HTTP response with body - 2 separators

void loadBalanceTraffic()
{
  char *buffer = (char *)malloc(sizeof(char) * BUFFER_INIT_SIZE);
  unsigned int buffer_size = BUFFER_INIT_SIZE;
  int next_server_index = 0;

  while (1) {
    // read request from client and copy it to buffer
    recv_msg(connfd_client, NO_BODY_REQUEST, &buffer, &buffer_size);

    // handle request by next server in cyclic line
    send_msg(connfd_server[next_server_index], buffer);
    recv_msg(connfd_server[next_server_index], RESPONSE, &buffer, &buffer_size);

    // send response back to client and get next server in cyclic line
    send_msg(connfd_client, buffer);
    next_server_index++;
    next_server_index = (next_server_index == SERVER_COUNT ? 0 : next_server_index);

    // connect new client session
    closeSession(connfd_client);
    connfd_client = acceptSession(sockfd_client);
  }
}

int main()
{
  srandom(time(0));  // Use current time as seed for random generator
  connectServersClient();
  loadBalanceTraffic();
  return 0;
}
