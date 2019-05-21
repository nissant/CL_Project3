#ifndef SOCKET_TOOLS
#define SOCKET_TOOLS

#define SERVER_COUNT 3
#define BUFFER_INIT_SIZE 512

int sockfd_client, connfd_client;
int sockfd_server, connfd_server[SERVER_COUNT];

void connectServersClient();
int acceptSession(int sokcet);
void closeSession(int socket);
int checkInvalidRequest(char *inBuffer, unsigned int msg_size);
int recv_msg(int socket, unsigned int msgSeparatorCount, char **msgBuffer, unsigned int *bufferSize,
             unsigned int *msgSize);
int send_msg(int connfd_client, char *buffer);
#endif