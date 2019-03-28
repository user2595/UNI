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
#include <time.h>
#include <sys/time.h>   
#include <sys/resource.h> 


// ________________________________________  FUNCTIONS START  ________________________________________


int connect_to_server (char* ip_address, char* port_no) {
    struct addrinfo hints, *server_address, *ptr;
    int sock_id;

// set 'hints'
    memset(&hints, 0, sizeof (struct addrinfo));    // set memory to 0
    hints.ai_family = AF_UNSPEC;                    // allow ip_v4 and ip_v6
    hints.ai_socktype = SOCK_STREAM;                // Datastream(TCP)

// execute 'getaddrinfo' -> fill in 'server_address' addrinfo using adress, portno., 'hints'   
    if (getaddrinfo(ip_address, portno, &hints, &server_address) != 0) {
        perror("Getaddrinfo failed\n"); 
        return -1;
    }

// loop : create a socket
    for(ptr = server_address ; ptr != NULL ; ptr = ptr->ai_next) {
    // execute 'socket' -> create socket 'sock_id' 
        if ((sock_id = socket(server_address->ai_family, server_address->ai_socktype, server_address->ai_protocol))== -1) {
            perror("Socket failed\n");
            continue;
        }
        server_address = ptr;
        break;
    }

// execute 'connect' -> assign 'sock_id' the server ip_address stored in 'server_address->ai_addr'
    if (connect(sock_id, server_address->ai_addr, server_address->ai_addrlen) == -1) {
        perror("Connect failed\n");
        close(sock_id);
        return -1;
    }

    freeaddrinfo(server_address);

// return 'socket_id' or -1 of failure 
    return sock_id;
}


// ________________________________________  MAIN START  ________________________________________




int main(int argc, char *argv[]) {
    int socket_id;


// errorcheck : argument count
    if (argc != 3) {
        fprintf(stderr,"Usage: hostname, port(17)\n");
        return 1;
    }
    


// create a socket accessable by 'socked_id'
    if((socket_id = connect_to_server(argv[1], argv[2])) == -1) {
        perror("Connect to Server failed\n");
        return -1;
    }



// free and close everything
	
	close(socket_id);


    return 0;
}
 
