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

    struct addrinfo hints, *server_address, *ptr;
    struct timespec time_start, time_finish;		
    int status, sockfd, bytes_received;
    long seconds, nanosecs;
    char answer[ANSWER_LEN];		

// errorcheck : argument count
    if (argc != 3) {
        fprintf(stderr,"usage: hostname, port(17)\n");
        return 1;
    }
    


// set 'hints'
    memset(&hints, 0, sizeof (struct addrinfo));	// set memory to 0
    hints.ai_family = AF_UNSPEC; 					// allow ip_v4 and ip_v6
    hints.ai_socktype = SOCK_STREAM;				// Datastream(TCP)

// execute 'getaddrinfo' -> fill in 'server_address' addrinfo using adress, portno., 'hints'   
    if (status = getaddrinfo(argv[1], argv[2], &hints, &server_address) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));	
        return 1;
    }

// loop : create a socket
    for(ptr = server_address ; ptr != NULL ; ptr = ptr->ai_next) {
    // execute 'socket' -> create socket 'sockfd' 
        if ((sockfd = socket(server_address->ai_family, server_address->ai_socktype, server_address->ai_protocol))== -1) {
        	perror("Socket");
        	continue;
        }
        server_address = ptr;
        break;
    }

// take time -> store in 'time_start'
    if (clock_gettime(CLOCK_REALTIME, &time_start) == -1) {
    	perror("Clock Start");
    	exit(1);
    }

// execute 'connect' -> assign 'sockfd' the server ip_address stored in 'server_address->ai_addr'
    if (connect(sockfd, server_address->ai_addr, server_address->ai_addrlen) == -1) {
	   	perror("connect failed");
	   	close(sockfd);
	   	exit(1);
   }

// loop : execute 'recv' until it returns 0, save received data in 'answer'
	while (bytes_received = recv(sockfd, answer, ANSWER_LEN-1,0)) {
		if (bytes_received == -1){
			perror("receive failed");
			close(sockfd);
			exit(1);
		} else if (bytes_received == 0) {
			break;
		} else {
	// print 'answer' with \0 terminating symbol at the end ('bytes_received'+1th position in 'answer' array)  
			answer[bytes_received] = '\0';
    		printf("%s", answer);			
		}
  	}

// take time -> store in 'time_finish'
  	if (clock_gettime(CLOCK_REALTIME,
  	 &time_finish) == -1) {
    	perror("Clock Finish");
    	exit(1);
    }

// calculate and print difference between 'time_start' and 'time_finish'
    seconds = time_finish.tv_sec - time_start.tv_sec;
    nanosecs = time_finish.tv_nsec - time_start.tv_nsec;
    if (nanosecs < 0) {
    	nanosecs += 1000000000;
    	seconds -= 1;
    }
    printf("\nTime elapsed: %e seconds\n ", (double)seconds + (double)nanosecs/(double)1000000000);



// free 'server_address'
	freeaddrinfo(server_address);

// close 'sockfd'	
	close(sockfd);

/* programm successfully connected to a server, received a message and 
   calculated the time the whole exchange took */
    return 0;
}
 
