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
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define LISTEN_BACKLOG 20   // Amount of clients 'listen' can handle waiting at once
#define DELETE 1
#define SET 2
#define GET 4

#define BIND 0
#define CONNECT 1

#define ID_LEN 1

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



// __________________________________________________  INTERN HASHTABLE  __________________________________________________



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



// __________________________________________________  HASHTABLE FUNCTIONS  __________________________________________________



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




// __________________________________________________  SEND/RECEIVE FUNCTIONS  __________________________________________________



// get a byte amount, an socket id and a pointer to a storage -> store the amount of bytes from 'recv' into the storage, return -1 on failure
int receive_bytes (int recv_amount, int socket, uint8_t *storage) {   
    int bytes_recv = 0;
    int bytes_recv_all = 0;     
    printf("  \nReceive Amount: %d\n  ", recv_amount);  // #DEBUG               
// loop : receive the full amount of bytes
    while(bytes_recv_all < recv_amount) {
        if((bytes_recv = recv(socket, &storage[bytes_recv_all], 1, 0)) == -1) {
            perror("Receive failed -1");
            close(socket);
            return -1;
        } else if (bytes_recv == 0) {
            continue;
        } else {
            printf("|  %d  ", storage[bytes_recv_all]); // #DEBUG
            bytes_recv_all += bytes_recv;   // count bytes received in 'bytes_recv_all'
            bytes_recv = 0;                 // redundant for safety
    
        }
    }
    printf("|\n"); // #DEBUG
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

        printf("Sendall - flag: %d flag_int: %d | key: %s value %s\n", flag, flag_intern, key, value); // #DEBUG


// execute 'send_bytes' -> send first header to client/peer
    if(send_bytes(6, socket, header) == -1) {
        perror("Send_Header failed");
        return -1;
    }

    if(flag_intern == 1) {
    // execute 'send_bytes' -> send second header to peer
        if(send_bytes(7, socket, header_2) == -1) {
            perror("Send_Header failed");
            return -1;
        }

    }

// execute 'send_bytes' -> send old key and new value to client/peer
    if(flag == GET || flag_intern) {
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


// __________________________________________________  OTHER FUNCTIONS  __________________________________________________



int create_socket(char* ip_addr, char * portnummer, int flag_socktype){
    int set_socked_flag = 1; // Wert > 0
    int sockfd;
    struct addrinfo hints, *res, *p;
        // Hints vorbereiten,
    memset(&hints, 0, sizeof (hints));
    hints.ai_family = AF_UNSPEC; // AF_INET or AF_INET6 to force version
    hints.ai_socktype = SOCK_STREAM;

    if(flag_socktype == BIND) {
        hints.ai_flags = AI_PASSIVE;//I'm telling the program to bind to the IP of the host it's running on.
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

    if(flag_socktype == BIND) {
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

  return (uint8_t)(hash % 256);
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




// __________________________________________________  MAIN FUNCTION  __________________________________________________



int main(int argc, char *argv[]) {

    int sock_listen, sock_client, sock_temp, sock_max, sock_predecessor = 0, sock_successor = 0;                       // socket identificators
    char *value_buffer, *key_buffer; // storage array for key and value
    uint8_t header[6], header_new[7];                   // storage arrays for 'header_old' and 'header_new'
    uint8_t my_ip[4];
    uint8_t my_id, predecessor_id;         // uint8 ID values
    uint8_t key_hashvalue;                                  // stores key in uint8 representation
    uint8_t flag, flag_intern, flag_client, flag_responsible = -1;   // 'flag' to indicate the request-type, 'flag_intern' to check if request is from client or peer
    uint16_t key_len, value_len;                        // length of key and value
    long my_ip_long;
    fd_set socket_read_set;
    unsigned long peer_ip;
    char peer_ip_string[20];
    int peer_port;
    char peer_port_string[10];


// errorcheck : argument count
    if (argc != 10) {
        perror("Wrong Usage: Our ID, Our IP, Our Port_no, Predecessor ID, Predecessor IP, Predecessor Port_no, Successor ID, Successor IP, Successor Port_no");
        return -1;
    }

// convert ip string into long


    // #ERROR : localhost


    my_ip_long = inet_addr(argv[2]);
    my_ip_long = htonl(my_ip_long);

    my_ip[0] = (uint8_t)(my_ip_long >> 24);
    my_ip[1] = (uint8_t)(my_ip_long >> 16);
    my_ip[2] = (uint8_t)(my_ip_long >> 8);
    my_ip[3] = (uint8_t)(my_ip_long);

    my_id = (uint8_t) strtol(argv[1], (char **)NULL, 10); // set ID | #ERROR : No Errorcheck, maybe unexpected behaviour
    predecessor_id = (uint8_t) strtol(argv[4], (char **)NULL, 10); // set ID | #ERROR : No Errorcheck, maybe unexpected behaviour



// create listener socket with id 'sock_listen'
    if((sock_listen = create_socket(NULL, argv[3], BIND)) == -1) {
        perror("Create Socket_listen failed");
        return -1;
    }

// execute 'listen' -> start listening for clients to connect
    if (listen(sock_listen, LISTEN_BACKLOG) == -1) 
    {
        perror("Listen failed");
        close(sock_listen);
        return -1;
    }    

// Wait until all peers are active
    sleep(5);  

// create successor socket with id 'sock_successor'
    if((sock_successor = create_socket(argv[8], argv[9], CONNECT)) == -1) {
        perror("Create Successor failed");
        return -1;
    }

// accept predecessor socket
    if((sock_predecessor = accept(sock_listen, (struct sockaddr * )NULL, (socklen_t * )0)) == -1) { 
        perror("Accept failed");
        return -1;
    }




// __________________________________________________  INFINITE LOOP  __________________________________________________



// infinite loop : Try to accept a client. If successful, answer all requests of the client until he closes the connection.
    while(1) {

        printf("\nRepeat Infinite Loop\n"); // #DEBUG
        sleep(1);

    // reset socket set for select to predecessor and listener
        FD_ZERO(&socket_read_set);
        FD_SET(sock_listen, &socket_read_set);
        FD_SET(sock_predecessor, &socket_read_set);

        if(sock_listen > sock_predecessor) {
            sock_max = sock_listen;
        } 
        else {
            sock_max = sock_predecessor;
        }

        if(FD_ISSET(sock_listen, &socket_read_set) && FD_ISSET(sock_predecessor, &socket_read_set)) {
            printf("##################################################\n"); // #DEBUG
        }

        if(select((sock_max + 1), &socket_read_set, NULL, NULL, NULL) < 0) { // chose which socket to work on next, no timeout
            perror("Select failure");
            continue;
        }

        printf("Nach Select\n"); // #DEBUG


        if(FD_ISSET(sock_listen, &socket_read_set)) {
        // execute 'accept' -> create 'sock_client' or return -1
            if((sock_client = accept(sock_listen, (struct sockaddr * )NULL, (socklen_t * )0)) == -1) { // #ERROR : Not sure about NULL / 0
                perror("Accept failed");
                continue;
            }
            else {
                flag_client = 1;
            }
        }

        else if (FD_ISSET(sock_predecessor, &socket_read_set)) {
            sock_client = sock_predecessor;
            flag_client = 0;
        }

        else {
            perror("Select failure empty");
            continue;
        }


// __________________________________________________  RECEIVE  __________________________________________________



    // receive first header-part in 'header'
        if(receive_bytes(6, sock_client, header) == -1) {
            perror("Receive_Header failed");
            break;
        }

    // interpret the header
        flag = header[0] & 7;                                         // check which type of request (GET/SET/DELETE)
        flag_intern = header[0] & (128);                           // check if client or peer request (intern?)
        if(flag_intern > 0){
            flag_intern = 1;
        }
        
    // receive or set the second part of header
        if(flag_intern == 1) {
        // receive second part in 'header_new'
            if(receive_bytes(7, sock_client, header_new) == -1) {
                perror("Receive_Header failed");
                break;
            }
        }
        else {
        // set second part in 'header_new'
            header_new[0] = my_id; // set ID | #ERROR : No Errorcheck, maybe unexpected behaviour
            for(int i = 0; i < 4; i++) {
                header_new[i+1] = my_ip[i];
            }
            uint16_t temp_portno = (uint16_t) strtol(argv[3], (char **)NULL, 10); // #ERROR : No Errorcheck, maybe unexpected behaviour
            header_new[5] = (uint8_t) (temp_portno >> 8);
            header_new[6] = (uint8_t) (temp_portno);
        }

    // calculate 'key_len' and 'value_len', reserve storage
        key_len = (uint16_t) header[3] + ((uint16_t)header[2] << 8);  // calculate 'key_len' 
        key_buffer = malloc(key_len+1);                               // reserve storage for 'key_buffer'
        value_len = (uint16_t) header[5] + ((uint16_t)header[4] << 8);// calculate 'value_len'
        value_buffer = malloc(value_len+1);                           // reserve storage for 'value_buffer'

    // receive full key in 'key_buffer', add \0 at the end
        if(receive_bytes(key_len, sock_client, (uint8_t * )key_buffer) == -1) {
            perror("Receive_Key failed");
            break;
        }
        key_buffer[key_len] = '\0';
        key_hashvalue = hash(key_buffer); // set key_hashvalue | #ERROR : HASHFUNCTION 

    // receive full value in 'value_buffer', add \0 at the end
        if(receive_bytes(value_len, sock_client, (uint8_t * )value_buffer) == -1) {
            perror("Receive_Value failed");
            break;
        }
        value_buffer[value_len] = '\0';


    // Case 2.2: Warst Empfänger aber nicht Hash_Verantwortlicher - ZURÜCK
    // Setze Internbit auf 0, Sende an Client aus Hash_Client!

        if(flag_client == 1 && flag_intern == 1) {
            printf("Case 2.2 - Return from Peer to Client\n"); // #DEBUG
            header[0] = header[0] & (0x8F);  //set Internbit auf 0            
            hashtable_print_client(); // #DEBUG
            sock_temp = hashtable_client_get((int)header[1]);
            send_all(sock_temp, header, header_new, (uint8_t *)key_buffer, (uint8_t *)value_buffer, flag, 0);
            close(sock_temp);
        }



// __________________________________________________  HANDLE REQUEST (HASHTABLE)  __________________________________________________


    // calculate if we are responsible for the request
        if(predecessor_id < key_hashvalue && key_hashvalue <= my_id) {
            flag_responsible = 1;
        } else if(predecessor_id > my_id && key_hashvalue <= my_id) {
            flag_responsible = 1;
        } else if (predecessor_id > my_id && key_hashvalue > predecessor_id) {
            flag_responsible = 1;
        } else {
            flag_responsible = 0;
        }

    // for the case that we receive the header back after being client-receiver, we do not hash again
        if(flag_intern == 1){
            if(header_new[0] == my_id) {
                flag_responsible = 0;
            }
        }

    // EXECUTE HASHTABLE FUNCTION
        if(flag_responsible == 1) {

        // execute DELETE operation
            if (flag == DELETE) {
                if(hashtable_delete(key_buffer) == 0) { 
                    header[0] = header[0] | 8;                  // Success : set acknoledgement bit
                }
                key_len = 0;                                    // no key to return
                value_len = 0;                                  // no value to return, redundant for safety
                header[2] = 0;
                header[3] = 0; 
                header[4] = 0; 
                header[5] = 0; 
            }

        // execute SET operation
            else if (flag == SET) {
                if(hashtable_set(key_buffer, value_buffer) == 0) {
                    header[0] = header[0] | 8;                              // Success : set acknoledgement bit
                }
                key_len = 0;                                                // no key to return
                value_len = 0;                                              // no value to return
                header[2] = 0;
                header[3] = 0; 
                header[4] = 0; 
                header[5] = 0; 
            }

        // execute GET
            else if (flag == GET) {
                free(value_buffer);
                if((value_buffer = hashtable_get(key_buffer)) != NULL) {    // #ERROR : Reuse of value_buffer
                // Success
                    header[0] = header[0] | 8;                  // set acknoledgement bit
                    value_len = strlen(value_buffer);    // set 'value_len' to lenght of new value string
                    header[4] = (uint8_t)(value_len >> 8);   // set MSB part of Value lenght in header   #ERROR: order?
                    header[5] = (uint8_t)(value_len);          // set LSB part of Value lenght in header
                } else {
                    key_len = 0;                                // no key to return
                    value_len = 0;                              // no value to return
                    header[2] = 0;
                    header[3] = 0; 
                    header[4] = 0; 
                    header[5] = 0; 
                }
            }

        // no flag was set
            else {
                perror("Flag was not found");
                break;
            }
        }


// __________________________________________________  ANSWER  __________________________________________________



// Case 1: nicht Empfänger und nicht Hash_Verantwortlicher
        // Send an Nachfolger

        if(flag_intern == 1 && flag_responsible == 0 && header_new[0] != my_id) {
            printf("Case 1\n"); // #DEBUG
            header[0] = header[0] | (1 << 7);
            send_all(sock_successor, header,  header_new, (uint8_t *)key_buffer, (uint8_t *)value_buffer, flag, 1);
        }


// Case 2.1: Empfänger aber nicht Hash_Verantwortlicher
        // Sende an Nachfolger, speicher Client in Hash_Client

        if(flag_intern == 0 && flag_responsible == 0) {
            printf("Case 2.1\n"); // #DEBUG
            hashtable_client_set((int)header[1], sock_client);
            hashtable_print_client(); // #DEBUG
            header[0] = header[0] | (128);
            send_all(sock_successor, header, header_new, (uint8_t *)key_buffer, (uint8_t *)value_buffer, flag, 1);
        }


// Case 3: nicht Empfänger aber Hash_Verantwortlicher
        // Sende an Empfänger direkt zurück

        if(flag_intern == 1 && flag_responsible == 1) {
            printf("Case 3 - Not responsible but Hash-Operator\n"); // #DEBUG
        // dont send key/value if they are not needed anymore
            if(flag != GET) {
                free(key_buffer);
                free(value_buffer);
                key_buffer = NULL;
                value_buffer = NULL;
            }
        // create new socket for answer to responsible peer, for that parse the ip address and portno. back to string
            peer_ip = ((unsigned long)header_new[1] << 24) + ((unsigned long)header_new[2] << 16) + ((unsigned long)header_new[3] << 8) + ((unsigned long)header_new[4]); // #ERROR
            peer_ip = htonl(peer_ip);
            inet_ntop(AF_INET, &peer_ip, peer_ip_string, INET_ADDRSTRLEN); // #ERROR
            peer_port = ((int)header_new[5] << 8) + ((int)header_new[6]);
            itoa(peer_port, peer_port_string);

            printf("Return IP Address: %s Port_no: %s\n", peer_ip_string, peer_port_string); // #DEBUG
            printf("Return IP Address: %ld Port_no: %d\n", peer_ip, peer_port); // #DEBUG

            sock_temp = create_socket(peer_ip_string, peer_port_string, CONNECT);
            send_all(sock_temp, header, header_new, (uint8_t *)key_buffer, (uint8_t *)value_buffer, flag, 1);
        }

// Case 4: Empfänger und Hash_Verantwortlicher
        // Sende an Client direkt

        if(flag_intern == 0 && flag_responsible == 1) {
            printf("Case 4\n"); // #DEBUG
            send_all(sock_client, header, header_new, (uint8_t *)key_buffer, (uint8_t *)value_buffer, flag, 0);
            close(sock_client);
        }

    }

    return 0;
}