
#define SERVER_COUNT 3
#define BUFFER_INIT_SIZE 1024

int sockfd_client, connfd_client;
int sockfd_server, connfd_server[SERVER_COUNT];

void connect_servers_client();