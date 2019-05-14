#include <stdio.h>
#include <stdlib.h> 
#include <string.h> 
#include <errno.h>
#include <time.h>

#include "socket_tools.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <netinet/in.h> 
#include <unistd.h>

#define LISTEN_COUNT 10
#define FAIL_PORT 80
#define LOWER_PORT 1024
#define UPPER_PORT 64000
#define SA struct sockaddr 

// Generates a random port number in range [lower, upper]. 
unsigned int getRandomPortInRange()
{
	if (random() % 2)
		return (unsigned int)FAIL_PORT;
	else
		return (unsigned int)((random() % (UPPER_PORT - LOWER_PORT + 1)) + LOWER_PORT);
}


void createBindSocket_LogPort(int *socket_ptr, FILE *file_log) {
	// socket create and verification 
	int new_socket;
	unsigned int port;
	struct sockaddr_in service;
	new_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (new_socket == -1) {
		printf("socket creation failed...\n");
	}
	else
		printf("Socket successfully created..\n");

	bzero(&service, sizeof(service));

	// assign IP, PORT 
	service.sin_family = AF_INET;
	service.sin_addr.s_addr = INADDR_ANY;	// inet_addr("127.0.0.1");
	port = getRandomPortInRange();
	service.sin_port = htons(port);	// Port will fail (80) half of the calls this will test the random port feature

	// Binding newly created socket to given IP and verification 
	while ((bind(new_socket, (SA*)&service, sizeof(service))) != 0) {
		int error_code = errno;
		//printf("Error code: %d\n", error_code);
		if (EADDRINUSE == error_code || EINVAL == error_code || EACCES == error_code) {
			printf("Port %d already in use, trying again...\n", FAIL_PORT);
			port = getRandomPortInRange();
			service.sin_port = htons(port);
		}
		else {
			printf("socket bind failed...\n");
		}
	}
	fprintf(file_log, "%u\n", port);
	fclose(file_log);
	*socket_ptr = new_socket;
	return;
}


void connect_servers_client() {
	FILE *server_port_fp;
	FILE *http_port_fp;
	// Create client/server sockets
	server_port_fp = fopen("server_port", "w");
	createBindSocket_LogPort(&sockfd_server, server_port_fp);
	http_port_fp = fopen("http_port", "w");
	createBindSocket_LogPort(&sockfd_client, http_port_fp);
	printf("Sockets successfully binded..\n");
	// Now server is ready to listen and verification 
	if ((listen(sockfd_client, LISTEN_COUNT)) != 0 || (listen(sockfd_server, LISTEN_COUNT)) != 0) {
		printf("Listen failed...\n");
		exit(-1);
	}
	else
		printf("LB listening..\n");
	// Accept the data packet from client and verification
	int i;
	for (i=0; i < SERVER_COUNT; i++) {
		connfd_server[i] = accept(sockfd_server, NULL, NULL);
		if (connfd_server[i] < 0) {
			printf("Server %d acccept failed...\n", i);
			exit(-1);
		}
	}
	printf("Servers successfully acccepted\n");
	connfd_client = accept(sockfd_client, NULL, NULL);
	if (connfd_client < 0) {
		printf("Client acccept failed...\n");
		exit(-1);
	}

	printf("Servers and client successfully acccepted\n");
}
