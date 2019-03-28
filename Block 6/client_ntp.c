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




#define UNIX_EPOCH 2208988800UL /* 1970 - 1900 in seconds */
#define OFFSET 2208988800ULL
#define DEBUG false
#define DEBUG_PRINT(x) if (DEBUG) {printf(x);}


#define FALSE 0
#define TRUE 1
#define PORT "123"
#define NUMBER_OF_MESSAGES 100
#define SLEEP 6
#define INTERN 0
#define EXTERN 1
#define ROOT_DISPERSION 2
// ---------------------------------------------------------------------------------------------------------------------
// ##################################################  Strucs  ##################################################
// ---------------------------------------------------------------------------------------------------------------------+
//Quelle Structs https://stackoverflow.com/questions/29112071/how-to-convert-ntp-time-to-unix-epoch-time-in-c-language-linux
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

typedef struct avg_server
{
    long double offset[NUMBER_OF_MESSAGES], delay[NUMBER_OF_MESSAGES];
    long double dispersion, avg_off, avg_delay, avg_root_dispersion;

}avg_server;
// ---------------------------------------------------------------------------------------------------------------------
// ##################################################  Help functions ##################################################
// ---------------------------------------------------------------------------------------------------------------------+


/* Linux man page bind() */
#define handle_error(msg)               \
    do {perror(msg); exit(EXIT_FAILURE);} while (0)

// convertiere zu hostorder und füge sekunden mit nanosek zusammen
long double convert_time(int flag, uint32_t sec, uint32_t fraction)
{
/*    printf("sec before conv. = %u, \tnano_sec before = %u\n",sec,nano_sec);
    printf("sec after conv. = %u, \tnano_sec after = %u\n",sec,nano_sec);
*/
    if(flag == EXTERN) {
		uint64_t fractionshift = (uint64_t) fraction;
        sec -= OFFSET;

        fractionshift *= 1000000; /* multiply by 1e6 */
    	fractionshift >>= 32;     /* and divide by 2^32 */
    	fraction =(uint32_t) fractionshift;
        return (long double)sec + (long double)fraction / (10000000000);

    }else if(flag == ROOT_DISPERSION){
        uint64_t fractionshift = (uint64_t) fraction;

        fractionshift *= 1000000; /* multiply by 1e6 */
    	fractionshift >>= 16;     /* and divide by 2^16 */
    	fraction =(uint32_t) fractionshift;
        return (long double)sec + (long double)fraction / (10000000);
    }


    //printf("result = %Lf\n",result);
    return (long double)sec + (long double)fraction / (1000000000);
}

 // berechne dispersion
void calculate_dispersion(avg_server* server){

    long double min = server->delay[0];
    long double max = server->delay[0];

    for (int message = 0; message < NUMBER_OF_MESSAGES; ++message)
    {
        if (min > server->delay[message]) {
            min = server->delay[message];
        }
        if (max < server->delay[message]) {
            max = server->delay[message];
        }
    }
    server->dispersion = max - min;
    //printf("Dispersion = %Lf \n " , server->dispersion);
}

void flip_dem_bits(server_send* a_server_send){

	a_server_send->server_li_vn_mode = (uint8_t) (ntohs(a_server_send->server_li_vn_mode) >> 8);
    a_server_send->server_stratum = (uint8_t) (ntohs(a_server_send->server_stratum ) >> 8);
    a_server_send->server_poll = (uint8_t) (ntohs(a_server_send->server_poll) >> 8);
    a_server_send->server_precision = (uint8_t) (ntohs(a_server_send->server_precision) >> 8);
    a_server_send->server_root_delay = ntohl(a_server_send->server_root_delay);
    a_server_send->server_root_dispersion = ntohl(a_server_send->server_root_dispersion );
    a_server_send->server_reference_timestamp_sec = ntohl(a_server_send->server_reference_timestamp_sec);
    a_server_send->server_reference_timestamp_microsec = ntohl(a_server_send->server_reference_timestamp_microsec);
   // a_server_send->server_originate_timestamp_sec = ntohl(a_server_send->server_originate_timestamp_sec);
    //a_server_send->server_originate_timestamp_microsec = ntohl(a_server_send->server_originate_timestamp_microsec);
    a_server_send->server_receive_timestamp_sec = ntohl(a_server_send->server_receive_timestamp_sec);
    a_server_send->server_receive_timestamp_microsec = ntohl(a_server_send->server_receive_timestamp_microsec);
    a_server_send->server_transmit_timestamp_sec = ntohl(a_server_send->server_transmit_timestamp_sec);
    a_server_send->server_transmit_timestamp_microsec= ntohl(a_server_send->server_transmit_timestamp_microsec);
}

// ---------------------------------------------------------------------------------------------------------------------
// ##################################################  Main  ##################################################
// ---------------------------------------------------------------------------------------------------------------------+
int main(int argc, char *argv[]) {

    int num_of_server = argc-1;
    fd_set readfds;
    struct addrinfo hints,*res, *p;
    struct timeval time_out;
    time_out.tv_sec=0, time_out.tv_usec = 0;
    struct sockaddr_storage server_address_rcv;
    int sockfd , bytes_send, best_server = 0;
    client_packet header_out;
    server_send header_in[num_of_server][NUMBER_OF_MESSAGES];
    avg_server avg_server_array[num_of_server];
    long double t1= 0, t2= 0, t3= 0, t4 = 0, r_dispersion = 0;
    struct timespec ts1, ts2;

    // ---------------------------------------------------------------------------------------------------------------------
    // ##################################################  Prepare Header_out ##################################################
    // ---------------------------------------------------------------------------------------------------------------------+
    memset(&avg_server_array, 0, sizeof(avg_server_array));
    memset( &header_in , 0 , sizeof header_in );
    memset( &header_out , 0 , sizeof header_out );

    // Setze NTP auf 4 und Mode auf 3
    header_out.client_li_vn_mode = 0b00100011;

    // ---------------------------------------------------------------------------------------------------------------------
    // ##################################################  outer for-loop() for each message  ###############################
    // ---------------------------------------------------------------------------------------------------------------------+

    for(int message = 0; message < NUMBER_OF_MESSAGES; message++){
    // ---------------------------------------------------------------------------------------------------------------------
    // ##################################################  Create Socket // Send Message   ##################################################
    // ---------------------------------------------------------------------------------------------------------------------

        for(int  server = 0; server < num_of_server; server++){
        	//FIRST TIMESSTAMP
            clock_gettime(CLOCK_REALTIME, &ts1);

            header_out.client_originate_timestamp_sec = (uint32_t) ts1.tv_sec;;
            header_out.client_originate_timestamp_microsec = (uint32_t) (ts1.tv_nsec );
            header_out.client_transmit_timestamp_sec = (uint32_t) ts1.tv_sec;
            header_out.client_transmit_timestamp_microsec = (uint32_t) (ts1.tv_nsec);


            memset( &hints , 0 , sizeof hints );
            hints.ai_family = AF_UNSPEC;
            hints.ai_socktype = SOCK_DGRAM;

            //printf("\nCreate Socket - ip: %s  port: %s \n", argv[server+1], PORT);
            if ((getaddrinfo(argv[server+1], PORT, &hints, &res)) < 0) {
                perror("getaddrinfo failed");
                return -1;
            }

            //  Iteriert durch Addrinfo Linked List
            for (p = res; p != NULL; p = p->ai_next) {
                if((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol))== -1) {
                    perror("Socket");
                    continue;
                }
                break;
            }
            //  Errorcheck p
            if (p == NULL) {
                perror("Socket_Null");
                return -1;
            }




            FD_ZERO(&readfds);
            while(!(FD_ISSET(sockfd, &readfds))){
         // execute 'sendto' -> activate server by sending datagram containing header
                time_out.tv_sec=2;
                if ((bytes_send = sendto(sockfd, &header_out, sizeof (header_out), 0, res->ai_addr, res->ai_addrlen)) < 1) {
                        perror("Send failed");
                        exit(1);
                }
                else{
                    //------------------------DEBUG-----------------
                    //printf("bytes_send = %d\n",bytes_send);
                    //printf("message number: %d\n",message);
                    // ---------------------------------------------
                }
                FD_ZERO(&readfds);
                FD_SET(sockfd, &readfds);
                select(sockfd + 1, &readfds, NULL,NULL, &time_out);
            }
    // ---------------------------------------------------------------------------------------------------------------------
    // ##################################################  Recv Message ##################################################
    // ---------------------------------------------------------------------------------------------------------------------
            socklen_t tmp = sizeof(server_address_rcv);
            			//printf("sleep\n");

            if((recvfrom(sockfd, &header_in[server][message], sizeof(header_in[server][message]), 0, (struct sockaddr *)&server_address_rcv, &tmp))==-1) {
                perror("Receive failed");
                close(sockfd);
                exit(1);
            }
            flip_dem_bits(&header_in[server][message]);


            // SET our second timestruct
            clock_gettime(CLOCK_REALTIME, &ts2);

            uint32_t client_rcv_timestamp_sec = (uint32_t) ts2.tv_sec;
            uint32_t client_rcv_timestamp_microsec = (uint32_t) (ts2.tv_nsec);
    // ---------------------------------------------------------------------------------------------------------------------
    // ##################################################  print (per message send)  ##################################################
    // ---------------------------------------------------------------------------------------------------------------------+


           // printf("\t Originate Timestamp \t %"PRIu32".%"PRIu32" \t %"PRIu32".%"PRIu32"\n",header_out.client_originate_timestamp_sec, header_out.client_originate_timestamp_microsec, header_in[server][message].server_originate_timestamp_sec, header_in[server][message].server_originate_timestamp_microsec);
            //printf("\t Transmit Timestamp \t %"PRIu32".%"PRIu32" \t %"PRIu32".%"PRIu32"\n",header_out.client_transmit_timestamp_sec,header_out.client_transmit_timestamp_microsec,header_in[server][message].server_transmit_timestamp_sec,header_in[server][message].server_transmit_timestamp_microsec);
            //printf("\t Receive Timestamp \t %"PRIu32".%"PRIu32" \t %"PRIu32".%"PRIu32"\n\n",client_rcv_timestamp_sec,client_rcv_timestamp_microsec,header_in[server][message].server_receive_timestamp_sec,header_in[server][message].server_receive_timestamp_microsec);


            // calculate avg's
            // Füge sekunden und nanosekunden zusammen
            t1 = convert_time(INTERN, header_out.client_transmit_timestamp_sec, header_out.client_transmit_timestamp_microsec);
            t2 = convert_time(EXTERN, header_in[server][message].server_receive_timestamp_sec, header_in[server][message].server_receive_timestamp_microsec);
            t3 = convert_time(EXTERN, header_in[server][message].server_transmit_timestamp_sec, header_in[server][message].server_transmit_timestamp_microsec);
            t4 = convert_time(INTERN, client_rcv_timestamp_sec,client_rcv_timestamp_microsec);
            uint32_t tmp2 = 0;
            r_dispersion = convert_time(ROOT_DISPERSION, tmp2 ,header_in[server][message].server_root_dispersion );

            //Berechne average offset und delay...

            //printf("T1 = %Lf, T2 = %Lf, T3= %Lf, T4= %Lf\n",t1, t2, t3, t4);



            //Wir speichern die offsetwerte und berechnen das durchschnittliche offset
            avg_server_array[server].offset[message] = (long double)(((t2-t1) + (t3-t4)) /2);
            avg_server_array[server].avg_off +=  (long double)((((t2-t1) + (t3-t4)) /2)/ NUMBER_OF_MESSAGES);

            //Wir speichern die Delaywerte und berechnen das durchschnittliche delay
            avg_server_array[server].delay[message] = (long double)((t4-t1) - (t3-t2));
            //printf("Delay message  %Lf\n", avg_server_array[server].delay[message]);
            avg_server_array[server].avg_delay += (long double)(((t4-t1) - (t3-t2)) / NUMBER_OF_MESSAGES);

            // berechne avg root dispersion und rechne von sekunden in mili
            avg_server_array[server].avg_root_dispersion += (r_dispersion / (NUMBER_OF_MESSAGES)) ;


            //prints für Aufgabe 6b:

            printf("%s, %Lf, %Lf, %Lf\n", argv[server+1], avg_server_array[server].offset[message], avg_server_array[server].delay[message], r_dispersion) ;



            close(sockfd);
            freeaddrinfo(res);
        }
    	sleep(SLEEP);
    }

    long double min_outer = 10000000;
    // berechne Dispersion für alle Server
    for (int server = 0; server < num_of_server; ++server){

        calculate_dispersion(&avg_server_array[server]);
       // printf("Dispersion = %Lf, avg_delay = %Lf, max = %Lf \n", avg_server_array[server].dispersion, min, max);
       //

        //printf("Root_dispersion in Sek = %Lf, \t avg_root_dispersion = %Lf\n",avg_server_array[server].avg_root_dispersion , avg_server_array[server].dispersion);
        if (min_outer > (avg_server_array[server].avg_root_dispersion + avg_server_array[server].dispersion))
        {
        	min_outer = (avg_server_array[server].avg_root_dispersion + avg_server_array[server].dispersion);
            best_server = server;
        }
    }

	//printf("\n\n \t ################################################\n");
	//printf("\t \t\tBest Server \t\t\t\n");
	//printf(" \t Hostname/IP = %s,\n \t AVG_Root_dispersion = %Lf,\n \t dispersion = %Lf,\n \t avg Delay = %Lf,\n \t avg_offset = %Lf  \n",argv[best_server+1], avg_server_array[best_server].avg_root_dispersion, avg_server_array[best_server].dispersion, avg_server_array[best_server].avg_delay, avg_server_array[best_server].avg_off);
	//printf("\n \t ################################################\n");


    clock_gettime(CLOCK_REALTIME, &ts2);

    uint32_t client_rcv_timestamp_sec = (uint32_t) ts2.tv_sec;
    uint32_t client_rcv_timestamp_microsec = (uint32_t) (ts2.tv_nsec);
    t1 = convert_time(INTERN, client_rcv_timestamp_sec,client_rcv_timestamp_microsec);

    //printf("\t Local Time: %Lf,\n \t avg_offset = %Lf, \n \t new local time : %Lf\n ", t1, avg_server_array[best_server].avg_off, t1 + avg_server_array[best_server].avg_off  );
	//printf("\n \t ################################################\n\n");

    return 0;
}
