#include "socket_tools.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define NO_BODY_REQUEST 1
#define RESPONSE 2

void loadBalanceTraffic()
{
  char *buffer = (char *)malloc(sizeof(char) * BUFFER_INIT_SIZE);
  unsigned int buffer_size = BUFFER_INIT_SIZE;
  int next_server_index = 0;

  while (1) {

    recv_msg(connfd_client, NO_BODY_REQUEST, &buffer, &buffer_size);

    send_msg(connfd_server[next_server_index], buffer);
    recv_msg(connfd_server[next_server_index], RESPONSE, &buffer, &buffer_size);

    send_msg(connfd_client, buffer);
    next_server_index++;
    next_server_index = (next_server_index == SERVER_COUNT ? 0 : next_server_index);

    closeSession(connfd_client);
    connfd_client = acceptSession(sockfd_client);
  }
}

int main()
{
  srandom(time(0));
  connectServersClient();
  loadBalanceTraffic();
  return 0;
}
