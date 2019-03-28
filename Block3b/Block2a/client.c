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

int main(int argc, char *argv[]) {


    struct addrinfo hints, *res;
    int status, sockfd;

    if (argc != 3) {
        fprintf(stderr,"usage: hostname, port(17)\n");
        return 1;
    }
    /*if(17 != (atoi(argv[2])))
   	{
   		fprintf(stderr,"port has to be 17 for quotas\n");
        return 1;
   	}
*/
    memset(&hints, 0, sizeof (struct addrinfo));
    hints.ai_family = AF_UNSPEC; // AF_INET or AF_INET6 to force version
    hints.ai_socktype = SOCK_STREAM;

    if ((status = getaddrinfo(argv[1], argv[2], &hints, &res)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return 1;
    }



    if((sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol))== -1)
    {
    	perror("Socket");
    	exit(1);
    }

    if(connect(sockfd, res->ai_addr, res->ai_addrlen) ==-1)
   {
   	perror("connect failed");
   	close(sockfd);
   	exit(1);
   }
      free(res);


//	int bytes_send;

	char answer[5000];
	int answer_len = 5000;
  int received;

while((received = recv(sockfd, answer, answer_len,0))){
		if (received == -1){
			perror("receive failed");
			close(sockfd);
			exit(1);
		}
    if (received == 0){
      break;
    }
    //printf("bytes received: %d\n", received );
    printf("%s", answer);
  }
		

	
	close(sockfd);

    return 0;
}