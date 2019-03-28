#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>
#include <math.h>
#include <sys/timeb.h>
#include <inttypes.h>
#include <limits.h>
#include <assert.h>
#include <stdbool.h>
#include <ctype.h>



#define UNIX_EPOCH 2208988800UL /* 1970 - 1900 in seconds */
#define OFFSET 2208988800ULL
#define DEBUG false
#define DEBUG_PRINT(x) if (DEBUG) {printf(x);}


#define FALSE 0
#define TRUE 1
#define HEADER_LEN 12           //in bytes
#define PORT "123"
#define NUMBER_OF_MESSAGES 8
// ---------------------------------------------------------------------------------------------------------------------
// ##################################################  Strucs  ##################################################
// ---------------------------------------------------------------------------------------------------------------------+
typedef struct client_packet client_packet;
struct client_packet {
  uint8_t client_li_vn_mode;
  uint8_t client_stratum;
  uint8_t client_poll;
  uint8_t client_precision;
  uint32_t client_root_delay;
  uint32_t client_root_dispersion;
  uint32_t client_reference_identifier;
  uint32_t client_reference_timestamp_sec;
  uint32_t client_reference_timestamp_microsec;
  uint32_t client_originate_timestamp_sec;
  uint32_t client_originate_timestamp_microsec;
  uint32_t client_receive_timestamp_sec;
  uint32_t client_receive_timestamp_microsec;
  uint32_t client_transmit_timestamp_sec;
  uint32_t client_transmit_timestamp_microsec;
}__attribute__((packed));

typedef struct server_send server_send;
struct server_send {
  uint8_t server_li_vn_mode;
  uint8_t server_stratum;
  uint8_t server_poll;
  uint8_t server_precision;
  uint32_t server_root_delay;
  uint32_t server_root_dispersion;
  char server_reference_identifier[4];
  uint32_t server_reference_timestamp_sec;
  uint32_t server_reference_timestamp_microsec;
  uint32_t server_originate_timestamp_sec;
  uint32_t server_originate_timestamp_microsec;
  uint32_t server_receive_timestamp_sec;
  uint32_t server_receive_timestamp_microsec;
  uint32_t server_transmit_timestamp_sec;
  uint32_t server_transmit_timestamp_microsec;
}__attribute__((packed));
// ---------------------------------------------------------------------------------------------------------------------
// ##################################################  Help functions ##################################################
// ---------------------------------------------------------------------------------------------------------------------+
void Prepare_Header(client_packet* Header){
  
  memset( &Header , 0 , sizeof Header );

  Header.client_li_vn_mode = 0b00100011;
  Header.client_stratum = 0;
  Header.client_poll = 0;
  Header.client_precision = 0;
  Header.client_root_delay = 0;
  Header.client_root_dispersion = 0;
  Header.client_reference_identifier = 0;
  Header.client_reference_timestamp_sec = 0;
  Header.client_reference_timestamp_microsec = 0;

  Header.client_receive_timestamp_sec = 0;
  Header.client_receive_timestamp_microsec = 0;
}

uint32_t ClockGetTime() {
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  return (uint32_t)ts.tv_sec * 1000000LL + (uint32_t)ts.tv_nsec / 1000LL;
}

/* Linux man page bind() */
#define handle_error(msg)               \
  do {perror(msg); exit(EXIT_FAILURE);} while (0)

// ---------------------------------------------------------------------------------------------------------------------
// ##################################################  Main  ##################################################
// ---------------------------------------------------------------------------------------------------------------------+
int main(int argc, char *argv[]) {
  time_t time_originate_sec;
  struct addrinfo hints,*res, *p;
  int sockfd , numbytes;
  int rv;
  client_packet Header;
  struct timeval time_out;
  struct timespec t1, t4, local_time;
  struct sockaddr_storage server_address_rcv;
  struct ServerCarry servers[argc-1];        
// ---------------------------------------------------------------------------------------------------------------------
// ##################################################  Prepare Header_out ##################################################
// ---------------------------------------------------------------------------------------------------------------------+
  
  Prepare_Header(&Header);

  server_send memrcv;
  memset( &memsend , 0 , sizeof memsend );
// ---------------------------------------------------------------------------------------------------------------------
// ##################################################  alliteration about the servers ##################################################
// ---------------------------------------------------------------------------------------------------------------------  

  for(int  i = 0; i < argc-1; i++){
// ---------------------------------------------------------------------------------------------------------------------
// ##################################################  Creat Socked  ##################################################
// ---------------------------------------------------------------------------------------------------------------------+
    memset( &hints , 0 , sizeof hints );
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    printf("\nCreate Socket - ip: %s  port: %s \n", argv[i+1], PORT); //DEBUG 

    if ( ( rv = getaddrinfo( "3.de.pool.ntp.org" , "123" , &hints , &res ) ) != 0 ) {
      perror( stderr , "getaddrinfo: %s\n" , gai_strerror(rv) );
      return 1;
    }

    // loop through all the results and make a socket
    for( p = res; p != NULL; p = p->ai_next ) {
      if ( ( sockfd = socket( p->ai_family , p->ai_socktype , p->ai_protocol ) ) == -1 ) {
    perror( "socket" );
    continue;
      }
      break;
    }

    if (p == NULL) {
      perror(stderr, "Error while binding socket\n");
      return 2;
    }
  // ---------------------------------------------------------------------------------------------------------------------
  // ##################################################  alliteration about the Message per servers##################################################
  // ---------------------------------------------------------------------------------------------------------------------
    for(int message = 0; message < NUMBER_OF_MESSAGES; message++){
      if (clock_gettime(CLOCK_REALTIME, &t1) == -1) {
                perror("Clock Start failed");
                exit(1);
      }
    /*time_originate_sec = time(NULL);
      Header.client_originate_timestamp_sec = time_originate_sec;
      Header.client_originate_timestamp_microsec = ClockGetTime(); */
  // ---------------------------------------------------------------------------------------------------------------------
  // ##################################################  Send Message ##################################################
  // ---------------------------------------------------------------------------------------------------------------------
      FD_ZERO(&readfds);
      while(!(FD_ISSET(sockfd, &readfds))){
    // execute 'sendto' -> activate server by sending datagram containing header
        time_out.tv_sec=2;
      
        if ( ( numbytes = sendto( sockfd, &memsend , sizeof memsend , 0 ,
                      p->ai_addr , p->ai_addrlen ) ) == -1 ) {
          perror("sendto");
          exit(1);
        }else{
          //------------------------DEBUG-----------------
          printf("bytes_send = %d\n",numbytes);
          printf("message number: %d\n",message);
          // ---------------------------------------------
        } 
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);

        select(sockfd + 1, &readfds, NULL,NULL, &time_out);

      }
    // ---------------------------------------------------------------------------------------------------------------------
    // ##################################################  Recv Message ##################################################
    // ---------------------------------------------------------------------------------------------------------------------
        if ( ( numbytes = recvfrom( sockfd , &memrcv , sizeof memrcv , 0 ,
                    (struct sockaddr *) &p->ai_addr, &p->ai_addrlen ) ) == -1 ) {
          perror( "recvfrom" );
          exit(1);
        }
    // take time -> store in 'time_finish'
        if (clock_gettime(CLOCK_REALTIME,&t4) == -1) {
          perror("Clock Finish failed");
          exit(1);
        }
        /*time_t time_rcv_sec = time(NULL);
        uint32_t client_rcv_timestamp_sec = time_rcv_sec;
        uint32_t client_rcv_timestamp_microsec = ClockGetTime();
        */
        freeaddrinfo(res);
    // ---------------------------------------------------------------------------------------------------------------------
    // ##################################################  print  ##################################################
    // ---------------------------------------------------------------------------------------------------------------------+
        char Identifier[5];
        memset(Identifier , '\0' , sizeof Identifier);
        memcpy(Identifier , memrcv.server_reference_identifier , sizeof memrcv.server_reference_identifier);

        printf("\t Reference Identifier \t %"PRIu32" \t\t\t %s\n",memsend.client_reference_identifier,Identifier);
        printf("\t Reference Timestamp \t %"PRIu32".%"PRIu32" \t\t\t %"PRIu32".%"PRIu32"\n",memsend.client_reference_timestamp_sec,memsend.client_reference_timestamp_microsec,memrcv.server_reference_timestamp_sec,memrcv.server_reference_timestamp_microsec);
        printf("\t Originate Timestamp \t %"PRIu32".%"PRIu32" \t %"PRIu32".%"PRIu32"\n",memsend.client_originate_timestamp_sec,memsend.client_originate_timestamp_microsec,memrcv.server_originate_timestamp_sec,memrcv.server_originate_timestamp_microsec);
        printf("\t Receive Timestamp \t %"PRIu32".%"PRIu32" \t %"PRIu32".%"PRIu32"\n",client_rcv_timestamp_sec,client_rcv_timestamp_microsec,memrcv.server_receive_timestamp_sec,memrcv.server_receive_timestamp_microsec);
        printf("\t Transmit Timestamp \t %"PRIu32".%"PRIu32" \t %"PRIu32".%"PRIu32"\n\n",memsend.client_transmit_timestamp_sec,memsend.client_transmit_timestamp_microsec,memrcv.server_transmit_timestamp_sec,memrcv.server_transmit_timestamp_microsec);

        close(sockfd);
    }
  
  }
    return 0;
}
/*#TODOconvert NTP to TV
      uint64_t aux = 0;
      1) NTP - Unix offset ((70*365 + 17)*86400 = 2208988800)
            aux -= OFFSET;
            tv->tv_sec = aux;
      2) convert the NTP time from network order to host order    
      3) calculate  TV-usec 
          aux *= 1000000; 
          aux >>= 32;     
          tv->tv_usec = aux;                            */