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

#define LISTEN_BACKLOG 128
#define PORT 17
#define BUF 1025 /* can change the buffer size as well */
#define LINES 512 /* change to accomodate other sizes, change ONCE here */

// Extrahiert eine Zufällige Zeile aus TXT Datei
// Max Zeilen = LINES Buchstaben = BUF

int countlines (FILE *fin)
{
    int  lines  = 0;
    char line[BUF];

    while(fgets(line, BUF, fin) != NULL) {
        lines++;
    }

    return lines;
}

char *get_rand_quote(char * filepath)
{
  FILE *fp;
  int i = 0;



 fp = fopen(filepath, "r");
  if (fp == NULL)
    {
        fprintf(stderr,"FILE Failed\n");
        exit(1);
    }


  int j = countlines(fp);
  if(j <= 0) 
      {
        fprintf(stderr,"FILE emptyn");
        exit(1);
    }

  fp = fopen(filepath, "r");
  if (fp == NULL)
    {
        fprintf(stderr,"FILE Failed\n");
        exit(1);
    }

    char list[j][BUF];

    while(fgets(list[i], BUF, fp)) {
        // get rid of ending \n from fgets 
      list[i][strlen(list[i])] = '\0';
       i++;
    }
  srand(time(NULL));
  char* quote = list[(rand()% i)];
  fclose(fp);
printf("%s\n",quote );
return quote;    

}

int main(int argc, char *argv[]) {
//server 1717 qotd.txt

    if (argc != 3) {
        fprintf(stderr,"(port) (txt.txt)\n");
        return 1;
    }
    
    struct addrinfo hints, *res;
    int status, sockfd, bytes_send;
    char* quote = get_rand_quote(argv[2]); 
    int yes =1;

    // Hints vorbereiten, 
    memset(&hints, 0, sizeof (hints));
    hints.ai_family = AF_UNSPEC; // AF_INET or AF_INET6 to force version
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;//I'm telling the program to bind to the IP of the host it's running on. 
    // Wichtig beim client / consumer
    //
    if ((status = getaddrinfo(NULL, argv[1], &hints, &res)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return 1;
    }


    if((sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol))== -1)
    {
    	perror("Socket");
    	exit(1);
    }
    // Resetet socket
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }
  

// Binding failt maybe wegen zu niedriger portnumber alles unter 1k -> sudo
   if(bind(sockfd, res->ai_addr, res->ai_addrlen)==-1)
   {
   	perror("binding failed maybe port too low?  ");
   	close(sockfd);
   	exit(1);
   }
   freeaddrinfo(res); // No usage anymore


  if (listen(sockfd, LISTEN_BACKLOG) == -1) 
   {
      perror("listen misses");
      close(sockfd);
      exit(1);
	}

/* Zum Prozesse eliminieren muss ich noch lesen
    ???????????????????????????????????????????????????????????
    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }
*/
  char answer[256];
  struct sockaddr_storage their_addr;

  while(1){
    int their_addr_id;
    int addrlen = sizeof(their_addr);
    if((their_addr_id = accept(sockfd, (struct sockaddr *)&their_addr,(socklen_t * ) &addrlen)) == -1)
    {
      perror("accept");
      continue;
    }
/*
    socklen_t receive =recv(sockfd, answer, strlen(answer), 0);
    if (receive == -1)
    {
      perror("receive failed");
      continue;
    }
    else if (receive == 0)
    {
      perror("receive aborted");
      continue;
    }

    
          memset(&answer, 0, strlen(answer));
*/
          if (!fork()) { // this is the child process
              close(sockfd); // child doesn't need the listener
              //bytessend
              if (send(their_addr_id, quote, strlen(quote), 0) == -1)
              {
              perror("Sending Failed");
              close(their_addr_id);
              exit(0);
              } 
              exit(EXIT_SUCCESS);
          }          
        

      
    close(their_addr_id);
		}

close(sockfd);


    return 0;
}