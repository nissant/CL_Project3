#ifndef SOCKET_TOOLS
#define SOCKET_TOOLS

#define SERVER_COUNT 3
#define BUFFER_INIT_SIZE 512

enum http_msg_type { NO_BODY_REQUEST = 1, RESPONSE = 2 };

int sockfd_client, connfd_client;
int sockfd_server, connfd_server[SERVER_COUNT];

void connectServersClient();
int acceptSession(int sokcet);
void closeSession(int socket);
int recv_msg(int socket, enum http_msg_type msgType, char **msgBuffer, unsigned int *bufferSize);
int send_msg(int connfd_client, char *buffer);
#endif