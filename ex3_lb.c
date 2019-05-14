#include <stdio.h>
#include <stdlib.h> 
#include <string.h> 
#include <errno.h>
#include <time.h>
#include "socket_tools.h"

// Function designed for chat between client and server. 

void load_balance_servers() 
{ 
    char msg_buffer[BUFFER_INIT_SIZE];
	int buffer_size = BUFFER_INIT_SIZE;
	int next_server_index = 0;
    // infinite loop for chat 
    while (1) { 
        bzero(msg_buffer, BUFFER_INIT_SIZE);
        
		get_client_request(connfd_client, msg_buffer, &buffer_size);	// read the message from client and copy it in buffer 

        printf("From client to L.B:\n %s ", msg_buffer);
		
		if (get_server_response(next_server_index, msg_buffer, buffer_size) == 0) {
			printf("Server %d disconnected! \n", next_server_index);
			return;
		}

		send_client_response(msg_buffer, buffer_size);
		next_server_index++;
		if (next_server_index == SERVER_COUNT) { next_server_index = 0; }
		wait_new_client();
    } 
} 


int main() { 
	srandom(time(0));		// Use current time as seed for random generator
	connect_servers_client();
	load_balance_servers();
    return 0;
} 

