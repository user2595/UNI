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

#define PORT "80"
#define LISTEN_BACKLOG 128
#define BUFFERSIZE 10000



int main(int argc, char *argv[]) {
// Manche Seiten Akzeptieren nur HTTPS,


    struct addrinfo hints, *res;
    int status, sockfd;
    if (argc != 2) {
        fprintf(stderr,"usage: hostname > datei\n");
        return 1;
    }
    char *position, *domain;
    char message[1000], path[1000], answer_buf[BUFFERSIZE];
    int  b_send, b_received;
    int total =0;
 


    // Trenne Domain Und Path
    if((position = strstr(argv[1], "//"))) position +=2;
    strcpy(path, (strchr(position, '/')));
    domain = strtok(position, "/");

    // Rufe Adressinformationen ab
    memset(&hints, 0, sizeof (struct addrinfo));
    hints.ai_family = AF_UNSPEC; // AF_INET or AF_INET6 to force version
    hints.ai_socktype = SOCK_STREAM;

    if ((status = getaddrinfo(domain, PORT, &hints, &res)) != 0) {
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
  freeaddrinfo(res);

// Speichere HTTP Get Message in message und Sende die Anfrage
  snprintf(message, 1026, "GET %s HTTP/1.1\r\nHOST: %s \r\n\r\n", path , domain);


  if((b_send = send(sockfd, message, strlen(message), 0))==-1){
      perror("Sending Failed");
      close(sockfd);
      exit(1);
    }
  
    // Empfange den ersten Teil der Daten mit header
   if ((b_received = recv(sockfd, answer_buf, BUFFERSIZE, 0)) == -1)
    {
    perror("receive failed");
    close(sockfd);
    exit(1);
    }

// Finde das Ende des Headers und setzte Pointer auf content_begin
char * header_end = strstr(answer_buf, "\r\n\r\n");
if (header_end != NULL)
{
  *header_end = '\0';
  header_end+=4;
}
//plus 4 wegen abgeschnittenem 4 Zeichen "\r\n\r\n"
int size_header = strlen(answer_buf) +4 ;

//get size of content 
char * temp = strstr(answer_buf, "Content-Length: ");
temp +=16;
size_t size = atoi(strtok(temp,"\r\n"));

// bytes received -  länge ansbuf  
int content_b_size = b_received - size_header;


// schreibe vom Ende des Headers die empfangenen Bytes ohne Header
fwrite(header_end, content_b_size , 1, stdout);

// der gesamte Content ohne header wird in total festgehalten
total += content_b_size;


// Solange Connenction nicht abbricht oder das gesate Bild Empfangen ist, empfange und drucke in STDOUT
while(1)
  {
    
    if ((b_received = recv(sockfd, answer_buf, BUFFERSIZE, 0)) == -1){
      perror("receive failed");
      close(sockfd);
      exit(1);
      }

    if (b_received==0){
      printf("break\n");
          break;
      }
    total += b_received;
    fwrite(answer_buf , b_received, 1, stdout);
    if (total >= size){
        break;
        }
  } 

  close(sockfd);
  
    return 0;
}