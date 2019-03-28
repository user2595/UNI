#define _POSIX_C_SOURCE 200112L

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <uthash.h>			// Hash-Table

#define LISTEN_BACKLOG 10	// how many clients 'listen' can handle waiting at once


int main(int argc, char *argv[]) {

    struct addrinfo hints, *my_addr, *ptr;	
    struct sockaddr_storage client_addr;
    int addrlen = sizeof(client_addr);			// used in 'accept'
    int setsockopt_flag = 1;					// used in 'setsockopt'		
    int sock_listen, sock_client, bytes_send;					



// errorcheck : argument count
    if (argc != 2) {
        fprintf(stderr,"(port)\n");
        return 1;
    }


// ______________________________  PROGRAM START  ______________________________


// set flags in 'hints'
    memset(&hints, 0, sizeof (hints));
    hints.ai_family = AF_UNSPEC; 
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    char answer[256];

// execute 'getaddrinfo' -> fill in 'server_address' addrinfo using adress, portno., 'hints'  
    if ((getaddrinfo(NULL, argv[1], &hints, &my_addr)) != 0) {
        perror("Getaddrinfo failed")
        return 1;
    }

// loop : create a socket
    for (ptr = my_addr; ptr != NULL; ptr = ptr->ai_next) {
    // execute 'socket' -> create socket 'sock_listen'
    	if ((sock_listen = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol)) == -1) {
    		perror("Socket failed");
    		continue;
    	} 
    	my_addr = ptr;
    	break;
    }

// execute 'setsockopt'	-> clean socket in case it has been used before
    if (setsockopt(sock_listen, SOL_SOCKET, SO_REUSEADDR, &setsockopt_flag, sizeof(int)) == -1) {
        perror("Setsockopt failed");
        close(sock_listen);
        exit(1);
    }

// execute 'bind' -> assign 'sock_listen' the port from 'my_addr'
   	if(bind(sock_listen, my_addr->ai_addr, my_addr->ai_addrlen)==-1) {
	   	perror("Binding failed");
	   	close(sock_listen);
	   	exit(1);
	}

// execute 'listen' -> start listening for clients to connect
    if (listen(sock_listen, LISTEN_BACKLOG) == -1) 
	{
    	perror("Listen failed");
    	close(sock_listen);
    	exit(1);
	}
  
// loop - infinite : Try to accept a client. If successful, answer all requests of the client until he closes the connection.
	while(1){
	    
	    if((sock_client = accept(sock_listen, (struct sockaddr * )&client_addr, (socklen_t * )&addrlen)) == -1) {
	    	perror("Accept failed");
	    	continue;
	    }
        
        


      
    	close(sock_client);
	}


// ______________________________  PROGRAM ENDS  ______________________________


	close(sock_listen);			// close 'sock_listen'
	freeaddrinfo(my_addr);		// free 'my_addr'

    return 0;
} 
