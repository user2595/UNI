#define _POSIX_C_SOURCE 200112L
#define ANSWER_LEN 1024

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


int main(int argc, char *argv[]) {

    struct addrinfo hints, *server_address, *pointer;
    struct timespec time_start, time_finish;
    struct sockaddr_storage server_address_rcv;	
    int status, sockfd, bytes_send, bytes_received;
    long seconds, nanosecs;
    char *ping = "YESS";	
    char answer[ANSWER_LEN];
    socklen_t server_address_len;   

// errorcheck : argument count
    if (argc != 3) {
        fprintf(stderr,"usage: hostname, port(17)\n");
        return 1;
    }
    

// set 'hints'
    memset(&hints, 0, sizeof (struct addrinfo));	// set memory to 0
    hints.ai_family = AF_UNSPEC; 					// allow ip_v4 and ip_v6
    hints.ai_socktype = SOCK_DGRAM;                  // Datagram (UDP)

// execute 'getaddrinfo' -> fill in 'server_address' addrinfo using ip-adress, portno. and 'hints'   
    if (status = getaddrinfo(argv[1], argv[2], &hints, &server_address) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));	
        return 1;
    }

// loop : create a socket
	for(pointer = server_address ; pointer != NULL ; pointer = pointer->ai_next) {
	// execute 'socket' -> create socket 'sockfd' or return -1
	    if ((sockfd = socket(pointer->ai_family, pointer->ai_socktype, pointer->ai_protocol))== -1) {
	    	perror("Socket failed");
	    	continue;
	    }
	    server_address = pointer;
	    break;
	}

// take time -> store in 'time_start'
    if (clock_gettime(CLOCK_REALTIME, &time_start) == -1) {
    	perror("Clock Start failed");
    	exit(1);
    }

// execute 'sendto' -> activate server by sending datagram containing 'ping'
    if ((bytes_send = sendto(sockfd, ping, strlen(ping), 0, server_address->ai_addr, server_address->ai_addrlen)) < 1) {
        perror("Send failed");
        exit(1);
    }

// set 'server_address_len' 
    server_address_len = sizeof(server_address_rcv);

// loop : execute 'recvfrom' until it returns 0, save received data in 'answer'
	while (bytes_received = recvfrom(sockfd, answer, ANSWER_LEN-1, 0, (struct sockaddr *)&server_address_rcv, &server_address_len)) {
		if (bytes_received == -1){
			perror("Receive failed");
			close(sockfd);
			exit(1);
		} else {
	// print 'answer' with \0 terminating symbol at the end ('received'+1th position in 'answer' array)  
			answer[bytes_received] = '\0';
    		printf("%s", answer);
    		break;			
		}
  	}

// take time -> store in 'time_finish'
  	if (clock_gettime(CLOCK_REALTIME,
  	 &time_finish) == -1) {
    	perror("Clock Finish failed");
    	exit(1);
    }

// calculate and print difference between 'time_start' and 'time_finish'
    seconds = time_finish.tv_sec - time_start.tv_sec;
    nanosecs = time_finish.tv_nsec - time_start.tv_nsec;
    if (nanosecs < 0) {
    	nanosecs += 1000000000;
    	seconds -= 1;
    }
    printf("\nTime elapsed: %e seconds\n", (double)seconds + (double)nanosecs/(double)1000000000);

// free 'server_address'
	freeaddrinfo(server_address);

// close 'sockfd'	
	close(sockfd);

/* programm successfully send a datagram to a server, received a datagram back and 
   calculated the time the whole exchange took */
    return 0;
}
 
