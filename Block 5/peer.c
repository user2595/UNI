#define _POSIX_C_SOURCE 200112L


#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netdb.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <stdint.h>         // Unsigned Int 8
#include "uthash.h"         // Hash-Table
#include <sys/time.h>       // Timeout
#include <time.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define TIMEOUT 5   // for the best tutor of them all

#define LISTEN_BACKLOG 20   // Amount of clients 'listen' can handle waiting at once

#define DELETE 1
#define SET 2
#define GET 4
#define ACKNOLEDGE 8
#define STABILIZE 16
#define NOTIFY 32
#define JOIN 64
#define INTERN 128

#define LISTEN 0
#define CONNECT 1

#define DIRECT 0
#define PREDECESSOR 1
#define SUCCESSOR 2

#define FALSE 0
#define TRUE 1


//Quellen:
// DJB2 (http://www.partow.net/programming/hashfunctions/#DJBHashFunction)


// struct : tuple inside the hashtable
typedef struct hashtable_element {
    char *key;
    char *value;
    UT_hash_handle hh;  // makes this structure hashable
}hashtable_element;


typedef struct hashtable_client {
    uint8_t trans_id;
    int socket_id;
    UT_hash_handle hh;  // #ERROR : maybe other handle needed
}hashtable_client;

hashtable_element *head = NULL;      // pointer to the whole hashtable    

hashtable_client *head_client = NULL;      // pointer to the whole hashtable    



// ------------------------------------------------------------------------------------------------------------------------
// ##################################################  CLIENT HASHTABLE  ##################################################
// ------------------------------------------------------------------------------------------------------------------------



// get a key, a value string and the hashtable_head-> add the tuple in the hashtable, return -1 on failure
int hashtable_client_set (int transaction_id, int socket_id) {
    struct hashtable_client *ptr = NULL;

    // check if the key already exists
    HASH_FIND_INT(head_client, &transaction_id, ptr);   // #ERROR : & ?
    if (ptr != NULL) {
        perror("Hashtable: Element already exists");
        return -1;
    }

// create new tuple
    ptr = (hashtable_client *) calloc(1, sizeof(hashtable_client));
    ptr->trans_id = transaction_id;
    ptr->socket_id = socket_id;

    printf("hashtable_client_set_POINTER trans_id: %d socket_id: %d\n", ptr->trans_id, ptr->socket_id);   // #DEBUG
    HASH_ADD_INT(head_client, trans_id, ptr); // insert 'ptr' in hashtable pointed to by 'head_client' #ERROR : trans_id or transaction id?
    return 0;
}

// get a key and the hashtable_head_client -> return related value and delete it from hashtable or return NULL
int hashtable_client_get (int transaction_id) {
    struct hashtable_client *ptr = NULL;
    int value_return;

    printf("hashtable_client_get: key: %d\n", transaction_id);   // #DEBUG 
    HASH_FIND_INT(head_client, &transaction_id, ptr);       // #ERROR : & ?
    if (ptr == NULL) {  
        perror("Client not found\n"); // #DEBUG #ERROR FINDET SOCKET ID NICHT
        return -1;
    } else {
        printf("Client found\n"); // #DEBUG
        HASH_DEL(head_client, ptr);
        value_return = ptr->socket_id;
        free(ptr);
        return value_return;
    }
}

// print all entries in hashtable
void hashtable_print_client() {
    struct hashtable_client *ptr;

    printf("hashtable_client print:\n");   // #DEBUG
    for (ptr = head_client ; ptr != NULL; ptr = (struct hashtable_client*)(ptr->hh.next)) {
        printf("  print: %d - %d\n", ptr->trans_id, ptr->socket_id);
    }
}



// ---------------------------------------------------------------------------------------------------------------------------
// ##################################################  HASHTABLE FUNCTIONS  ##################################################
// ---------------------------------------------------------------------------------------------------------------------------



// get a key, a value string and the hashtable_head-> add the tuple in the hashtable, return -1 on failure
int hashtable_set (char *key, char *value) {
    struct hashtable_element *ptr = NULL;
    int key_len = strlen(key);
    int value_len = strlen(value);


    printf("hashtable_set key: %s value: %s\n key_len: %d value_len: %d\n", key, value, key_len, value_len);   // #DEBUG
    // check if the key already exists
    HASH_FIND_STR(head, key, ptr);
    if (ptr != NULL) {
        perror("Hashtable: Element already exists");
        return -1;
    }

    // create new tuple
    ptr = (hashtable_element * ) malloc(sizeof (hashtable_element));
    ptr->key = malloc((key_len + 1) * sizeof(char));
    strcpy(ptr->key, key);
    ptr->value = malloc((value_len + 1) * sizeof(char));
    strcpy(ptr->value, value);
    printf("hashtable_set_POINTER key: %s value: %s\n", ptr->key, ptr->value);   // #DEBUG
    HASH_ADD_KEYPTR( hh, head, ptr->key, key_len, ptr); // insert 'ptr' in hashtable pointed to by 'head'
    return 0;
}

// get a key and the hashtable_head -> return related value or NULL
char *hashtable_get (char *key) {
    struct hashtable_element *ptr = NULL;

    printf("hashtable_get: key: %s\n", key);   // #DEBUG
    HASH_FIND_STR(head, key, ptr);
    if (ptr == NULL) {
        printf("not found\n"); // #DEBUG
        return NULL;
    } else {
        printf("found\n"); // #DEBUG
        return ptr->value;
    }
}

// get a key and the hashtable_head -> delete the related tuple in hashtable or return -1
int hashtable_delete (char *key) {
    struct hashtable_element *ptr = NULL;

    printf("hashtable_delete:\n");   // #DEBUG
    HASH_FIND_STR(head, key, ptr);
    if (ptr == NULL) {
        return -1;
    } else {
        HASH_DEL(head, ptr);
        free(ptr->key);
        free(ptr->value);
        free(ptr);
        return 0;
    }
}

// print all entries in hashtable
void hashtable_print() {
    struct hashtable_element *ptr;

    printf("hashtable_print:\n");   // #DEBUG
    for (ptr = head ; ptr != NULL; ptr = (struct hashtable_element*)(ptr->hh.next)) {
        printf("  print: %s - %s\n", ptr->key, ptr->value);
    }
}



// ---------------------------------------------------------------------------------------------------------------------
// ##################################################  SEND/RECEIVE FUNCTIONS  #########################################
// ---------------------------------------------------------------------------------------------------------------------



// get a byte amount, an socket id and a pointer to a storage -> store the amount of bytes from 'recv' into the storage, return -1 on failure
int receive_bytes (int recv_amount, int socket, uint8_t *storage) {   
    int bytes_recv = 0;
    int bytes_recv_all = 0;
    if(recv_amount > 0) {
        printf("\n    ");  // #DEBUG               
    }     
// loop : receive the full amount of bytes
    while(bytes_recv_all < recv_amount) {
        if((bytes_recv = recv(socket, &storage[bytes_recv_all], 1, 0)) == -1) {
            perror("Receive failed -1");
            close(socket);
            return -1;
        } else if (bytes_recv == 0) {
            continue;
        } else {
            printf(" | %d ", storage[bytes_recv_all]); // #DEBUG
            bytes_recv_all += bytes_recv;   // count bytes received in 'bytes_recv_all'
            bytes_recv = 0;                 // redundant for safety
    
        }
    }
    if(recv_amount > 0) {
        printf("|\n"); // #DEBUG              
    }
    return 0;
}


// get a byte amount, an socket id and a pointer to a storage -> send the amount of bytes via the socket to the connected device, return -1 on failure
int send_bytes (int send_amount, int socket, uint8_t *storage) {
    int bytes_send = 0;
    int bytes_send_all = 0;
// loop : send the full amount of bytes
    while(bytes_send_all < send_amount) {
        if((bytes_send = send(socket, storage, (send_amount- bytes_send_all), 0)) == -1) {
            perror("Send failed");
            close (socket);
            return -1;
        } else if (bytes_send == 0) {
            perror("Send failed");
            close (socket);
            return -1;
        } else {
            bytes_send_all += bytes_send;   // count bytes send in 'bytes_send_all'
            bytes_send = 0;                 // redundant for safety
        }
    }
    return 0;
}

int send_all (int socket, uint8_t *header, uint8_t *header_2, uint8_t *key, uint8_t *value, uint8_t flag, uint8_t flag_intern) {

        printf("\nSEND %d flag: %d flag_int: %d | key: %s value %s\n", socket, flag, flag_intern, key, value); // #DEBUG
        printf("    Header: | %d | %d | %d | %d | %d | %d |\n", header[0], header[1], header[2], header[3], header[4], header[5]);
        printf("    Header_2: | %d | %d | %d | %d | %d | %d | %d | %d |\n\n", header_2[0], header_2[1], header_2[2], header_2[3], header_2[4], header_2[5], header_2[6], header_2[7]);


// execute 'send_bytes' -> send first header to client/peer
    if(send_bytes(6, socket, header) == -1) {
        perror("Send_Header failed");
        return -1;
    }

    if(flag_intern == TRUE) {
    // execute 'send_bytes' -> send second header to peer
        if(send_bytes(8, socket, header_2) == -1) {
            perror("Send_Header failed");
            return -1;
        }

    }

// execute 'send_bytes' -> send old key and new value to client/peer
    if(flag == GET || flag_intern == TRUE) {
        if(key != NULL) {
            if(send_bytes(strlen((char *)key), socket, key) == -1) {    // send old key 
                perror("Send_Key failed");
                return -1;
            }
        }
        if(value != NULL) {
            if(send_bytes(strlen((char *)value), socket, value) == -1) {  // send looked-up value
                perror("Send_Value failed");
                return -1;
            }
        }
    }
    return 0;
}



// ---------------------------------------------------------------------------------------------------------------------
// ##################################################  OTHER FUNCTIONS  ##################################################
// ---------------------------------------------------------------------------------------------------------------------



int create_socket(char* ip_addr, char * portnummer, int flag_socktype){
    int set_socked_flag = 1; // Wert > 0
    int sockfd;
    struct addrinfo hints, *res, *p;
        // Hints vorbereiten,
    memset(&hints, 0, sizeof (hints));
    hints.ai_family = AF_UNSPEC; // AF_INET or AF_INET6 to force version
    hints.ai_socktype = SOCK_STREAM;

    printf("\nCreate Socket - ip: %s  port: %s  flag_socktype: ", ip_addr, portnummer);
    if(flag_socktype != 0) {
        printf("Connect\n");
    } else {
        printf("Listen\n");
    }

    if(flag_socktype == LISTEN) {
        hints.ai_flags = AI_PASSIVE;//I'm telling the program to LISTEN to the IP of the host it's running on.
        if ((getaddrinfo(NULL, portnummer, &hints, &res)) < 0) {
            perror("getaddrinfo failed");
            return -1;
        }
    }
    else {
        if ((getaddrinfo(ip_addr, portnummer, &hints, &res)) < 0) {
            perror("getaddrinfo failed");
            return -1;
        }
    }


// Iteriert durch Addrinfo Linked List
    for (p = res; p != NULL; p = p->ai_next) {
        if((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol))== -1) {
            perror("Socket");
            continue;
        }
        break;
    }

// Errorcheck p    
    if (p == NULL) {
        perror("Socket_Null");
        return -1;
    }

// Resetet socket falls local Address schon in benutzung
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &set_socked_flag,sizeof(int)) == -1) {
        perror("setsockopt");
        return -1;
    }

    if(flag_socktype == LISTEN) {
    // execute 'bind' -> assign 'sockfd' the servers own ip_address stored in 'server_address->ai_addr'
        if(bind(sockfd, p->ai_addr, p->ai_addrlen)==-1){
            perror("binding failed maybe port too low?  ");
            close(sockfd);
            return -1;
        }
    } else if(flag_socktype == CONNECT) {
    // execute 'connect' -> assign 'sockfd' the server ip_address stored in 'server_address->ai_addr'
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            perror("Connect failed\n");
            close(sockfd);
            return -1;
        }
    }
   freeaddrinfo(res);
   return sockfd;
}



// #ERROR : ?
uint8_t hash (const char *str)
{
  int hash = 5381;
  int i = 0;
  int length = strlen(str);

  for (i = 0; i < length; ++str, ++i)
    {
      hash = ((hash << 5) + hash) + (*str);
    }

  return (uint8_t)(hash % (256*256));
}

 void reverse(char s[])
 {
     int i, j;
     char c;

     for (i = 0, j = strlen(s)-1; i<j; i++, j--) {
         c = s[i];
         s[i] = s[j];
         s[j] = c;
     }
}  


void itoa(int n, char s[])
 {
     int i, sign;

     if ((sign = n) < 0)  /* record sign */
         n = -n;          /* make n positive */
     i = 0;
     do {       /* generate digits in reverse order */
         s[i++] = n % 10 + '0';   /* get next digit */
     } while ((n /= 10) > 0);     /* delete it */
     if (sign < 0)
         s[i++] = '-';
     s[i] = '\0';
     reverse(s);
}  


// #ERROR
void set_headernew(uint8_t* header_new, uint16_t id, uint8_t ip[], uint8_t port[]) {

    header_new[0] = (uint8_t)(id >> 8);
    header_new[1] = (uint8_t)id;
    header_new[2] = ip[0];
    header_new[3] = ip[1];
    header_new[4] = ip[2];
    header_new[5] = ip[3];
    header_new[6] = port[0];
    header_new[7] = port[1];
}


// #ERROR
void set_header_lens_to_0(uint8_t* header) {
    header[2] = 0;
    header[3] = 0;
    header[4] = 0;
    header[5] = 0;
}



















// ---------------------------------------------------------------------------------------------------------------------
// ##################################################  MAIN FUNCTION  ##################################################
// ---------------------------------------------------------------------------------------------------------------------



int main(int argc, char *argv[]) {

    int sock_listen, sock_connector, sock_temp, sock_predecessor = -1, sock_successor = -1;                       // socket identificators
    uint16_t key_hashvalue;                                  // stores key in uint8 representation
    char *value_buffer = NULL, *key_buffer = NULL; // storage array for key and value
    uint8_t flag, flag_intern, flag_connecttype, flag_responsible = -1;   // 'flag' to indicate the request-type, 'flag_intern' to check if request is from client or peer
    uint8_t header[6] = {0}, header_new[8];                   // storage arrays for 'header_old' and 'header_new'
    uint8_t my_ip[4], my_port[2];         // uint8 ID values
    uint8_t predecessor_ip[4] = {0}, predecessor_port[2] = {0};
    uint8_t  successor_ip[4] = {0}, successor_port[2] ={0};
    uint16_t my_id = 0, successor_id = 0,predecessor_id = 0;
    long my_ip_long;
    unsigned long peer_ip;
    int peer_port;
    char peer_ip_string[20], peer_port_string[10];

    uint16_t key_len, value_len;                        // length of key and value
    int max_sockid, select_return;
    fd_set socket_read_set;
    struct timeval timeout;
        timeout.tv_sec = TIMEOUT;
        timeout.tv_usec = 0;


// errorcheck : argument count
    if (argc != 3 && argc != 4 && argc != 6) {
        perror("Wrong Usage: Our IP, Our Port_no, (Our ID), (Peer IP, Peer Port_no)");
        return -1;
    }
    
    printf("\n##################################################"); // #DEBUG - Create new block
    printf("\n######################## START ###################"); // #DEBUG - Create new block
    printf("\n##################################################\n\n"); // #DEBUG - Create new block


    // #TODO : DEAL WITH SUCCESOR CLOSING SOCKET AFTER MISMATCHING STABILIZE



// __________________________________________________ INTERPRET ARGVs ________________________________________


// create listener socket
    if((sock_listen = create_socket(NULL, argv[2], LISTEN)) == -1) {
        perror("Create Socket_listen failed");
        return -1;
    }
    if (listen(sock_listen, LISTEN_BACKLOG) == -1) {
        perror("Listen failed");
        close(sock_listen);
        return -1;
    }    


// convert my ip string into long        
    my_ip_long = inet_addr(argv[1]);    // #ERROR : localhost
    my_ip_long = htonl(my_ip_long);     
    my_ip[0] = (uint8_t)(my_ip_long >> 24);
    my_ip[1] = (uint8_t)(my_ip_long >> 16);
    my_ip[2] = (uint8_t)(my_ip_long >> 8);
    my_ip[3] = (uint8_t)(my_ip_long);

// convert my portno. to uint8_t
    uint16_t temp_portno = (uint16_t) strtol(argv[2], (char **)NULL, 10); // #ERROR : No Errorcheck, maybe unexpected behaviour
    my_port[0] = (uint8_t) (temp_portno >> 8);
    my_port[1] = (uint8_t) (temp_portno);

// seperate different argument-cases -> contact peer or leave 'sock_successor' = 0, also set 'my_id'
    if(argc == 3) {
        my_id = 0;
    }
    else if(argc == 4) {
        my_id = (uint16_t) strtol(argv[3], (char **)NULL, 10);
    }


// __________________________________________________ MY JOIN REQUEST ________________________________________


    else if(argc == 6 ) {
        printf("\n__________OUTGOING JOIN__________\n\n"); // #DEBUG
        my_id = (uint16_t) strtol(argv[3], (char **)NULL, 10);
        header[0] = (JOIN + INTERN);     // JOIN Flag + Intern Bit
        set_headernew(header_new, my_id, my_ip, my_port);

    // create socket to known peer to join
        if((sock_temp = create_socket(argv[4], argv[5], CONNECT)) == -1) {  
            perror("Connect Join failed");
            return -1;
        }
    // send join request
        send_all(sock_temp, header, header_new, NULL, NULL, JOIN, 1);

        printf("CLOSE Sock_Temp\n");
        close(sock_temp);

        printf("\nACCEPT Successor\n");
        if((sock_successor = accept(sock_listen, (struct sockaddr * )NULL, (socklen_t * )0)) == -1) { // #ERROR : Not sure about NULL / 0
            perror("Accept failed");
            return -1;
        }
        printf("\nRECEIVE\n");
    // receive answer to join request  
        if(receive_bytes(6, sock_successor, header) == -1) {
            perror("Join_Receive_Header failed");
            return -1;
        }
        if(receive_bytes(8, sock_successor, header_new) == -1) {
            perror("Join_Receive_Header_new failed");
            return -1;
        }
    // errorcheck
        if(header[0] != (JOIN + ACKNOLEDGE + INTERN)) {
            perror("Join failed, maybe your ID was already taken");
            return -1;
        }

        successor_id = ((uint16_t)header_new[0] << 8) + header_new[1];
        successor_ip[0] = header_new[2];
        successor_ip[1] = header_new[3];
        successor_ip[2] = header_new[4];
        successor_ip[3] = header_new[5];
        successor_port[0] = header_new[6];
        successor_port[1] = header_new[7];

        printf("\nSUCCESSOR: %d | %d | %d | %d | %d | %d | %d \n", successor_id, successor_ip[0], successor_ip[1], successor_ip[2], successor_ip[3], successor_port[0], successor_port[1]);

// __________________________________________________ CREATE STARTING SOCKETS ________________________________________


    // reset both header
        memset(header, 0, 6);
        memset(header_new, 0, 8);
    }



// ---------------------------------------------------------------------------------------------------------------------
// ##################################################  INFINITE LOOP  ##################################################
// ---------------------------------------------------------------------------------------------------------------------



// infinite loop : Try to accept a client. If successful, answer all requests of the client until he closes the connection.
    while(1) {

        // printf("\nInfinite Loop\n"); // #DEBUG
        sleep(1);


// __________________________________________________ SELECT ________________________________________


    // set socket set for select to predecessor, successor and listener
        FD_ZERO(&socket_read_set);
        FD_SET(sock_listen, &socket_read_set);
        if(sock_predecessor > 0) { FD_SET(sock_predecessor, &socket_read_set); }
        if(sock_successor > 0)   { FD_SET(sock_successor, &socket_read_set); }
        max_sockid = sock_listen + sock_predecessor + sock_successor + 2;

        printf("\n##################################################\n"); // #DEBUG - Create new block
        printf("Predecessor: | %d | %d | %d | %d | %d | %d | %d | %d |\n", sock_predecessor, predecessor_id, predecessor_ip[0], predecessor_ip[1], predecessor_ip[2], predecessor_ip[3], predecessor_port[0], predecessor_port[1]);
        printf("Successor: | %d | %d | %d | %d | %d | %d | %d | %d |\n", sock_successor, successor_id, successor_ip[0], successor_ip[1], successor_ip[2], successor_ip[3], successor_port[0], successor_port[1]);


    // select which socket to work with next, also time the next stabilize
        if((select_return = select((max_sockid + 1), &socket_read_set, NULL, NULL, &timeout)) < 0) { // chose which socket to work on next, no timeout
            perror("Select failure");
            continue;
        } else if (select_return == 0) {
            printf("\nTIMEOUT\n"); //#DEBUG
        // Outgoing STABILIZE
            if(sock_successor >= 0) {
                printf("\n__________OUTGOING STABILIZE__________\n\n");
                header[0] = (STABILIZE+INTERN);
                set_headernew(header_new, my_id, my_ip, my_port);
                send_all(sock_successor, header, header_new, NULL, NULL, STABILIZE, 1);
            } else if (sock_predecessor >= 0) {
                printf("\n__________OUTGOING STABILIZE (Predecessor)__________\n\n");
                header[0] = (STABILIZE+INTERN);
                set_headernew(header_new, my_id, my_ip, my_port);
                send_all(sock_predecessor, header, header_new, NULL, NULL, STABILIZE, 1);
            }
            timeout.tv_sec = TIMEOUT;
            continue;
        }
        printf("\n"); // #DEBUG


// __________________________________________________ EVALUATE SELECT ________________________________________


        if(FD_ISSET(sock_listen, &socket_read_set)) {
        // accept -> create 'sock_connector' or return -1
            if((sock_connector = accept(sock_listen, (struct sockaddr * )NULL, (socklen_t * )0)) == -1) { // #ERROR : Not sure about NULL / 0
                perror("Accept failed");
                continue;
            }
            else {
                flag_connecttype = DIRECT;
            }
        }
        else if (FD_ISSET(sock_successor, &socket_read_set)) {
            sock_connector = sock_successor;
            flag_connecttype = SUCCESSOR;
        }

        else if (FD_ISSET(sock_predecessor, &socket_read_set)) {
            sock_connector = sock_predecessor;
            flag_connecttype = PREDECESSOR;
        }


        else {
            perror("Select failure empty");
            continue;
        }




        printf("RECEIVE: %d\n    ", sock_connector);

        if(flag_connecttype == 0) {
            printf("flag_connecttype: DIRECT\n");
        }
        if(flag_connecttype == 1) {
            printf("flag_connecttype: PREDECESSOR\n");
        }
        if(flag_connecttype == 2) {
            printf("flag_connecttype: SUCCESSOR\n");
        }


// ---------------------------------------------------------------------------------------------------------------
// ##################################################  RECEIVE  ##################################################
// ---------------------------------------------------------------------------------------------------------------


    // reset both header
        memset(header, 0, 6);
        memset(header_new, 0, 8);


// __________________________________________________ RECEIVE HEADER ________________________________________


    // receive first header-part in 'header'
        if(receive_bytes(6, sock_connector, header) == -1) {
            perror("Receive_Header failed");
            break;
        }

    // evaluate header
        flag = header[0] & 119;                                         // check which type of request (GET/SET/DELETE/JOIN/NOTIFY/STABILIZE)
        flag_intern = header[0] & (INTERN);                                // check if client or peer request (intern?)
        if(flag == 0) {
            perror("Flag is 0");
            continue;
        }
        if(flag_intern > 0) {
            flag_intern = TRUE;
        }
        
    // receive or set the second part of header
        if(flag_intern == TRUE) {
        // receive second part in 'header_new'
            if(receive_bytes(8, sock_connector, header_new) == -1) {
                perror("Receive_Header failed");
                break;
            }
        }
        else {
        // set second part in 'header_new'
            set_headernew(header_new, my_id, my_ip, my_port);
        }

    // calculate 'key_len' and 'value_len', reserve storage
        key_len = (uint16_t) header[3] + ((uint16_t)header[2] << 8);  // calculate 'key_len' 
        key_buffer = malloc(key_len+1);                               // reserve storage for 'key_buffer'
        value_len = (uint16_t) header[5] + ((uint16_t)header[4] << 8);// calculate 'value_len'
        value_buffer = malloc(value_len+1);                           // reserve storage for 'value_buffer'

        printf("key_len: %d value_len: %d\n", key_len, value_len);

// __________________________________________________ RECEIVE KEY & VALUE ________________________________________


    // receive full key in 'key_buffer', add \0 at the end
        if(receive_bytes(key_len, sock_connector, (uint8_t * )key_buffer) == -1) {
            perror("Receive_Key failed");
            break;
        }
        key_buffer[key_len] = '\0';
        key_hashvalue = hash(key_buffer); // set key_hashvalue | #ERROR : HASHFUNCTION 

    // receive full value in 'value_buffer', add \0 at the end
        if(receive_bytes(value_len, sock_connector, (uint8_t * )value_buffer) == -1) {
            perror("Receive_Value failed");
            break;
        }
        value_buffer[value_len] = '\0';

        printf("\n    Flag: %d Flag_intern: %d\n", flag, flag_intern);
        // printf("Finished Receiving\n");


// __________________________________________________ RETURN FROM PEER & STABILIZE ________________________________________



    //  Set Internbit 0, send back to Client from Hash_Client!
        if(flag_intern == TRUE && (flag <= GET) && ((uint16_t)header_new[0] << 8) + header_new[1] == my_id) {  // #ERROR : Flag inconsistancies
            printf("Case 2.2 - Return from Peer to Client (Direct)\n"); // #DEBUG
            header[0] = header[0] & (0x8F);  //set Internbit to 0            
            hashtable_print_client(); // #DEBUG
            sock_temp = hashtable_client_get((int)header[1]);
            send_all(sock_temp, header, header_new, (uint8_t *)key_buffer, (uint8_t *)value_buffer, flag, 0);
            printf("CLOSE Sock_Temp\n");
            close(sock_temp);
            continue;
        }

    // Incoming STABILIZE
        if(flag == STABILIZE) {
            printf("\n__________INCOMING STABILIZE__________\n\n"); // #DEBUG
            header[0] = (NOTIFY+INTERN);        // Set flag to Notify, flag_intern to True
            if(sock_predecessor < 0) {     // we dont have a predecessor yet, adopt connector as predecessor
                sock_predecessor = sock_connector;
                predecessor_id = ((uint16_t)header_new[0] << 8) + header_new[1];
                predecessor_ip[0] = header_new[2];
                predecessor_ip[1] = header_new[3];
                predecessor_ip[2] = header_new[4];
                predecessor_ip[3] = header_new[5];
                predecessor_port[0] = header_new[6];
                predecessor_port[1] = header_new[7];
                printf("\nNEW PREDECESSOR: | %d | %d | %d | %d | %d | %d | %d |\n\n", predecessor_id, predecessor_ip[0], predecessor_ip[1], predecessor_ip[2], predecessor_ip[3], predecessor_port[0], predecessor_port[1]);
            // Outgoing NOTIFY
                printf("\n__________OUTGOING NOTIFY__________\n\n"); // #DEBUG
                header[0] = (NOTIFY+INTERN);
                send_all(sock_connector, header, header_new, NULL, NULL, NOTIFY, 1);
                continue;
            }
            else if (flag_connecttype == DIRECT) {  // reply with predecessor
            // Outgoing NOTIFY
                printf("Stabilizer was NOT Predecessor\n");
                printf("\n__________OUTGOING NOTIFY__________\n\n"); // #DEBUG
                header[0] = (NOTIFY+INTERN);
                set_headernew(header_new, predecessor_id, predecessor_ip, predecessor_port);
                send_all(sock_connector, header, header_new, NULL, NULL, NOTIFY, 1);
                continue;
            }
            else if (flag_connecttype == PREDECESSOR) { // reply to predecessor with his own header_new
            // Outgoing NOTIFY
                printf("Stabilizer was Predecessor\n");
                printf("\n__________OUTGOING NOTIFY__________\n\n"); // #DEBUG
                header[0] = (NOTIFY+INTERN);
                send_all(sock_connector, header, header_new, NULL, NULL, NOTIFY, 1);
                continue;
            } else if (flag_connecttype == SUCCESSOR && sock_predecessor == sock_successor) {
                printf("Stabilizer was Predecessor (and Successor)\n");
                continue;
            } else if (flag_connecttype == SUCCESSOR && sock_predecessor != sock_successor) {
                printf("Stabilizer was Successor\n");
                printf("\n__________OUTGOING NOTIFY__________\n\n"); // #DEBUG
                header[0] = (NOTIFY+INTERN);
                set_headernew(header_new, predecessor_id, predecessor_ip, predecessor_port);
                send_all(sock_connector, header, header_new, NULL, NULL, NOTIFY, 1);
                continue;
            }
            printf("None of the Incoming Stabilize Cases applied\n");
            continue;
        }


// __________________________________________________ INCOMING NOTIFY ________________________________________


    // Incoming NOTIFY
        if(flag == NOTIFY && flag_connecttype == SUCCESSOR) {
            printf("\n__________INCOMING NOTIFY__________\n\n"); // #DEBUG
            if(((uint16_t)header_new[0] << 8) + header_new[1] == my_id) {
                printf("Nothing changed\n");
                continue;
            } else {
            // switch to new Successor!
                successor_id = ((uint16_t)header_new[0] << 8) + header_new[1];
                successor_ip[0] = header_new[2];
                successor_ip[1] = header_new[3];
                successor_ip[2] = header_new[4];
                successor_ip[3] = header_new[5];
                successor_port[0] = header_new[6];
                successor_port[1] = header_new[7];
                if(sock_successor != sock_predecessor) {
                    printf("CLOSE Sock_Successor\n");
                    close(sock_successor);
                } else {
                    printf("NO CLOSE (Successor = Predecessor)\n"); 
                }
                
            // connect to new Successor, send a stabilize so he knows i am trying to be his predecessor

                peer_ip = ((unsigned long)header_new[2] << 24) + ((unsigned long)header_new[3] << 16) + ((unsigned long)header_new[4] << 8) + ((unsigned long)header_new[5]); // #ERROR
                peer_ip = htonl(peer_ip);
                inet_ntop(AF_INET, &peer_ip, peer_ip_string, INET_ADDRSTRLEN); // #ERROR
                peer_port = ((int)header_new[6] << 8) + ((int)header_new[7]);
                itoa(peer_port, peer_port_string);

                printf("New Successor IP Address: %s Port_no: %s\n", peer_ip_string, peer_port_string); // #DEBUG
            //  printf("New Successor IP Address: %ld Port_no: %d\n", peer_ip, peer_port); // #DEBUG

                sock_successor = create_socket(peer_ip_string, peer_port_string, CONNECT);      // #ERROR : does Successor accept me as predecessor?
                header[0] = (STABILIZE+INTERN);    
                set_headernew(header_new, my_id, my_ip, my_port);
                send_all(sock_successor, header, header_new, NULL, NULL, STABILIZE, 1);
                continue;
            }
        }


    // Errorcheck
        if(flag == NOTIFY && flag_connecttype == PREDECESSOR) {
            printf("\n__________INCOMING NOTIFY (from Predecessor)__________\n\n");
            successor_id = predecessor_id;
            successor_ip[0] = predecessor_ip[0];
            successor_ip[1] = predecessor_ip[1];
            successor_ip[2] = predecessor_ip[2];
            successor_ip[3] = predecessor_ip[3];
            successor_port[0] = predecessor_port[0];
            successor_port[1] = predecessor_port[1];
            sock_successor = sock_predecessor;

        printf("\nSUCCESSOR & PREDECESSOR (second Peer): | %d | %d | %d | %d | %d | %d | %d |\n\n", successor_id, successor_ip[0], successor_ip[1], successor_ip[2], successor_ip[3], successor_port[0], successor_port[1]);


            continue;
        }

        if(flag == NOTIFY && flag_connecttype == DIRECT) {
            printf("\n__________INCOMING NOTIFY (Direct)__________\n\n");
            successor_id = predecessor_id;
            successor_ip[0] = predecessor_ip[0];
            successor_ip[1] = predecessor_ip[1];
            successor_ip[2] = predecessor_ip[2];
            successor_ip[3] = predecessor_ip[3];
            successor_port[0] = predecessor_port[0];
            successor_port[1] = predecessor_port[1];
            sock_successor = sock_predecessor;

        printf("\nSUCCESSOR & PREDECESSOR (second Peer): | %d | %d | %d | %d | %d | %d | %d |\n\n", successor_id, successor_ip[0], successor_ip[1], successor_ip[2], successor_ip[3], successor_port[0], successor_port[1]);


            continue;
        }


// __________________________________________________  JOIN - FIRST PART  ________________________________________


    // if the join has a duplicate (my) id
        if(flag == JOIN && ((uint16_t)header_new[0] << 8) + header_new[1] == my_id) {
            perror("Join with my ID - duplicate");
            send_all(sock_connector, header, header_new, NULL, NULL, JOIN, 1);
            close(sock_connector);
            continue;
        }
    // Check if we are responsible by using key_hashvalue
        else if (flag == JOIN) {
            key_hashvalue = ((uint16_t)header_new[0] << 8) + header_new[1];
        }




// ----------------------------------------------------------------------------------------------------------------------------------
// ##################################################  HANDLE REQUEST (HASHTABLE)  ##################################################
// ----------------------------------------------------------------------------------------------------------------------------------

        //printf("Responsibility calculation: \n");

    // calculate if we are responsible for the request
        if(sock_predecessor < 0 && sock_successor < 0) { // #ERROR
            flag_responsible = TRUE;
        }else if(predecessor_id < key_hashvalue && key_hashvalue <= my_id) {
            flag_responsible = TRUE;
        } else if(predecessor_id > my_id && key_hashvalue <= my_id) {
            flag_responsible = TRUE;
        } else if (predecessor_id > my_id && key_hashvalue > predecessor_id) {
            flag_responsible = TRUE;
        } else {
            flag_responsible = FALSE;
        }

        printf("\nFlag responsible : %d\n", flag_responsible);

    // for the case that we receive the header back after being client-receiver, we do not hash again
        if(flag_intern == TRUE && flag <= GET){
            if(((uint16_t)header_new[0] << 8) + header_new[1] == my_id) {
                flag_responsible = FALSE;
            }
        }


// __________________________________________________  HASHTABLE FUNCTION  ________________________________________


        if(flag_responsible == TRUE) {

        // DELETE
            if (flag == DELETE) {
                if(hashtable_delete(key_buffer) == 0) { 
                    header[0] = header[0] | ACKNOLEDGE;                  // Success : set acknoledgement bit
                }
                key_len = 0;                                    // no key to return
                value_len = 0;                                  // no value to return, redundant for safety
                set_header_lens_to_0(header);      // #ERROR
            }

        // SET
            else if (flag == SET) {
                if(hashtable_set(key_buffer, value_buffer) == 0) {
                    header[0] = header[0] | ACKNOLEDGE;                              // Success : set acknoledgement bit
                }
                key_len = 0;                                                // no key to return
                value_len = 0;                                              // no value to return
                set_header_lens_to_0(header);      // #ERROR
            }

        // GET 
            else if (flag == GET) {
                free(value_buffer);
                if((value_buffer = hashtable_get(key_buffer)) != NULL) {    // #ERROR : Reuse of value_buffer
                // Success
                    header[0] = header[0] | ACKNOLEDGE;                  // set acknoledgement bit
                    value_len = strlen(value_buffer);    // set 'value_len' to lenght of new value string
                    header[4] = (uint8_t)(value_len >> 8);   // set MSB part of Value lenght in header   #ERROR: order?
                    header[5] = (uint8_t)(value_len);          // set LSB part of Value lenght in header
                } else {
                    value_len = 0;                              // no value to return
                    header[4] = 0;
                    header[5] = 0;      // #ERROR
                }
            }



// __________________________________________________  JOIN - RESPONSIBLE  ________________________________________


        // Incoming JOIN
            else if (flag == JOIN && flag_responsible == TRUE) {
            // Set new Predecessor values
                if (flag_connecttype == DIRECT) {
                    printf("\n__________INCOMING JOIN__________\n\n");
                } else {
                    printf("\n__________INCOMING JOIN (Intern)__________\n\n");
                }

            // create socket to predecessor
                peer_ip = ((unsigned long)header_new[2] << 24) + ((unsigned long)header_new[3] << 16) + ((unsigned long)header_new[4] << 8) + ((unsigned long)header_new[5]); // #ERROR
                peer_ip = htonl(peer_ip);
                inet_ntop(AF_INET, &peer_ip, peer_ip_string, INET_ADDRSTRLEN); // #ERROR
                peer_port = ((int)header_new[6] << 8) + ((int)header_new[7]);
                itoa(peer_port, peer_port_string);

                sock_temp = sock_predecessor;

            // create new predecessor
                sock_predecessor = create_socket(peer_ip_string, peer_port_string, CONNECT);

                printf("DEBUG: Socktemp: %d Sockpre %d \n", sock_temp, sock_predecessor);

            // inform our old predecessor he has new successor
                if(sock_temp > 0) {
                    header[0] = (NOTIFY+INTERN);
                    send_all(sock_temp, header, header_new, NULL, NULL, NOTIFY, 1);
                }

            // set new predecessor values
                predecessor_id = ((uint16_t)header_new[0] << 8) + header_new[1];
                predecessor_ip[0] = header_new[2];
                predecessor_ip[1] = header_new[3];
                predecessor_ip[2] = header_new[4];
                predecessor_ip[3] = header_new[5];
                predecessor_port[0] = header_new[6];
                predecessor_port[1] = header_new[7];

            // send to new predecessor my id/ip/port
                header[0] = (JOIN + ACKNOLEDGE + INTERN);
                set_headernew(header_new, my_id, my_ip, my_port);
                send_all(sock_predecessor, header, header_new, NULL, NULL, JOIN, 1);

                if(sock_successor < 0) {
                    printf("\n__________OUTGOING STABILIZE (to Predecessor)__________\n\n");
                    header[0] = (STABILIZE + INTERN);
                    send_all(sock_predecessor, header, header_new, NULL, NULL, STABILIZE, 1);
                }

                continue;
            }

        // no flag was set
            else {
                perror("Flag was not found");
                break;
            }
        }

// --------------------------------------------------------------------------------------------------------------
// ##################################################  ANSWER  ##################################################
// --------------------------------------------------------------------------------------------------------------


// Case 0: Nicht Verantwortlicher für ein Join Request

        if(flag_responsible == FALSE && flag == JOIN) {
            printf("\nCASE 0 - Send Join Request to Successor\n");
            header[0] = header[0] | INTERN;
            send_all(sock_successor, header, header_new, NULL, NULL, JOIN, 1);
            continue;
        }





// Case 1: nicht Empfänger und nicht Hash_Verantwortlicher
        // Send an Nachfolger

        if(flag_intern == TRUE && flag_responsible == FALSE && ((uint16_t)header_new[0] << 8) + header_new[1] != my_id && flag <= GET) {
            printf("CASE 1\n"); // #DEBUG
            header[0] = header[0] | (INTERN);
            send_all(sock_successor, header,  header_new, (uint8_t *)key_buffer, (uint8_t *)value_buffer, flag, 1);
        }


// Case 2.1: Empfänger aber nicht Hash_Verantwortlicher
        // Sende an Nachfolger, speicher Client in Hash_Client

        if(flag_intern == FALSE && flag_responsible == FALSE && flag <= GET) {
            printf("CASE 2.1\n"); // #DEBUG
            hashtable_client_set((int)header[1], sock_connector);
            hashtable_print_client(); // #DEBUG
            header[0] = header[0] | (INTERN);
            send_all(sock_successor, header, header_new, (uint8_t *)key_buffer, (uint8_t *)value_buffer, flag, 1);
        }


// Case 3: nicht Empfänger aber Hash_Verantwortlicher
        // Sende an Empfänger direkt zurück

        if(flag_intern == TRUE && flag_responsible == TRUE && flag <= GET) {
            printf("CASE 3 - Not responsible but Hash-Operator\n"); // #DEBUG
        // dont send key/value if they are not needed anymore
            if(flag != GET) {
                free(key_buffer);
                free(value_buffer);
                key_buffer = NULL;
                value_buffer = NULL;
            }

            if(((uint16_t)(header_new[0] << 8) + header_new[1]) == predecessor_id) {
                send_all(sock_predecessor, header, header_new, (uint8_t *)key_buffer, (uint8_t *)value_buffer, flag, 1);
            }
            else if(((uint16_t)(header_new[0] << 8) + header_new[1]) == successor_id) {
                send_all(sock_successor, header, header_new, (uint8_t *)key_buffer, (uint8_t *)value_buffer, flag, 1);
            }
            else{
            // create new socket for answer to responsible peer, for that parse the ip address and portno. back to string
                peer_ip = ((unsigned long)header_new[2] << 24) + ((unsigned long)header_new[3] << 16) + ((unsigned long)header_new[4] << 8) + ((unsigned long)header_new[5]); // #ERROR
                peer_ip = htonl(peer_ip);
                inet_ntop(AF_INET, &peer_ip, peer_ip_string, INET_ADDRSTRLEN); // #ERROR
                peer_port = ((int)header_new[6] << 8) + ((int)header_new[7]);
                itoa(peer_port, peer_port_string);

                printf("Return IP Address: %s Port_no: %s\n", peer_ip_string, peer_port_string); // #DEBUG
            //printf("Return IP Address: %ld Port_no: %d\n", peer_ip, peer_port); // #DEBUG

                sock_temp = create_socket(peer_ip_string, peer_port_string, CONNECT);
                send_all(sock_temp, header, header_new, (uint8_t *)key_buffer, (uint8_t *)value_buffer, flag, 1);
            }



        
        }

// Case 4: Empfänger und Hash_Verantwortlicher
        // Sende an Client direkt

        if(flag_intern == FALSE && flag_responsible == TRUE && flag <= GET) {
            printf("CASE 4\n"); // #DEBUG
            send_all(sock_connector, header, header_new, (uint8_t *)key_buffer, (uint8_t *)value_buffer, flag, 0);
            printf("CLOSE Sock_Connector\n");
            close(sock_connector);
        }

    }

    return 0;
}