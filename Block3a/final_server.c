#define _POSIX_C_SOURCE 200112L

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include "unistd.h"
#include <ctype.h>
#include <time.h>
#include "uthash.h"

#define LISTEN_BACKLOG 128



typedef struct hashnode //dictionary node
{
    char *key;
    char *value;
    uint16_t value_len;
    uint16_t key_len;
    UT_hash_handle hh; /* makes this structure hashable */

}hashnode;

struct hashnode *head = NULL;
// add key value pair to hasmap
void add_movie(char *key, char *value, int key_len, int value_len) {
    struct hashnode *s;

    HASH_FIND_STR(head, key, s);// head->first node of Linkedlist; key->string to search for key; s-> output/* id already in the hash? */
    if (s==NULL) {

        s = (struct hashnode *)malloc(sizeof *s);

        s-> key_len = key_len;
        s-> value_len = value_len;

        s->key = malloc(key_len*sizeof (char));
        strncpy(s->key, key, key_len);
        s->value = malloc(value_len*sizeof(char));
        strncpy(s->value, value, value_len);

        HASH_ADD_KEYPTR( hh, head, s->key, strlen(s->key), s ); // insert "s" in hashtable "hh" instead of "key"

    }
}

// Suche Movie in Hashtable, wenn keine Movie vorhanden return NULL
struct hashnode *find_movie(char *key) {
    struct hashnode *s = NULL;

    HASH_FIND_STR( head, key, s );  /* s: output pointer */
    if (s != NULL)
    {
        return s;
    }
    else return NULL;
}
// löscht einen eintrag
void delete_movie(struct hashnode *movie) {
    HASH_DEL(head, movie); /* iterate über Hasmape   */
    free(movie->key);
    free(movie->value);  /* movie: pointer to deletee */
    free(movie);
}
//löscht alle einträge
void delete_all() {
  struct hashnode *current_movie, *tmp;

  HASH_ITER(hh, head, current_movie, tmp) { /* iterate über Hasmape   */
    HASH_DEL(head, current_movie);  /* delete it (movies advances to next) */
    free(current_movie);             /* free it */
  }
}
// printed hasmap
void print_movies() {
    struct hashnode *s;

    for(s=head; s != NULL; s=(struct hashnode*)(s->hh.next)) {
        printf("movie id %s: name %s\n", s->key, s->value);
    }
}
// sorted by value
int value_sort(struct hashnode *a, struct hashnode *b) {
    return strcmp(a->value,b->value); // return: >0= a<b; <= = a>b; 0=a==B;
}
// sorted by key
int key_sort(struct hashnode *a, struct hashnode *b) {
    return strcmp(a->key, b->key); // return: >0= a<b; <= = a>b; 0=a==B;
}
//siehe oben
void sort_by_name() {
    HASH_SORT(head, value_sort);
}

//siehe oben
void sort_by_key() {
    HASH_SORT(head, key_sort);
}


    //Holt locale Addressinfos, erstellt und bindet Port auf übergebene PortNR
int bind_socket(char * portnummer){
    int yes = 1; // Bel Wert > 0
    int status, sockfd;
    struct addrinfo hints, *res, *p;
        // Hints vorbereiten,
        memset(&hints, 0, sizeof (hints));
        hints.ai_family = AF_UNSPEC; // AF_INET or AF_INET6 to force version
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = AI_PASSIVE;//I'm telling the program to bind to the IP of the host it's running on.

        if ((status = getaddrinfo(NULL, portnummer, &hints, &res)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return 1;
        }
    // Iteriert durch Addrinfo Linked List
    for (p = res; p != NULL; p = p->ai_next)
    {
        if((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol))== -1)
        {
            perror("Socket");
            continue;
        }
    // Resetet socket falls local Address schon in benutzung
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }


        // Binding failt maybe wegen zu niedriger portnumber alles unter 1k -> sudo
        if(bind(sockfd, p->ai_addr, p->ai_addrlen)==-1){
            perror("binding failed maybe port too low?  ");
            close(sockfd);
            continue;
        }
    }
   freeaddrinfo(res);
   return sockfd;
}






int main(int argc, char *argv[]) {

    if (argc != 2)
    {
        perror("Please give PORT as parameter");
        exit(1);
    }


    // holt Adressinformationen vom localhost und bindet Socket an definierten PORT
    int sockfd = bind_socket(argv[1]);


// EXECUTE LISTEN
  if (listen(sockfd, LISTEN_BACKLOG) == -1)
   {
      perror("listen misses");
      close(sockfd);
      exit(1);
    }



// Initialisiere Adressinformationen vom Client
    struct sockaddr_storage their_addr;
    int their_addr_id;
    int addrlen = sizeof(their_addr);
    int b_send;


//
  while(1)
  {
        // initialisiere Key und Value
        char *key_buffer, *value_buffer;
        size_t key_len, value_len;

        // Array mit fester Reihenfolge in das die Header informationen gespeichert werden
        uint8_t header[6];
        socklen_t header_size = (socklen_t) sizeof(header);
        socklen_t bytes_recv_all = 0, bytes_recv = 0; // received bytes für while Schleife
        //Flag für auszuführende Operation
        uint8_t flag = 0;
        // ACCEPT
        if((their_addr_id = accept(sockfd, (struct sockaddr *)&their_addr,(socklen_t * ) &addrlen)) == -1)
        {
          perror("accept");
          continue;
        }

        //  Empfange den Header in uint8 header Array
        while(bytes_recv_all < header_size) {
            if((bytes_recv = recv(their_addr_id, &header[bytes_recv_all], 1, 0)) == -1) {
                perror("Receive failed");
                continue;
            } else if (bytes_recv == 0) {
                perror("Receive failed");
                continue;
            } else {
                bytes_recv_all += bytes_recv;   // count amount of bytes received in this while-loop in 'bytes_recv_all'
            }
        } bytes_recv_all = 0; // Wieder null für die nächste schleife


        // setze flag für auszuführende Aktion  wenn NULL Kehre zur while zurück
        if((flag = header[0] & 0x7) == 0) {
            perror("Flag = 0");
            continue; // verunde die drei höchsten Bit mit 00000111 -> 1, 2 ,4 da die reihenfolge sorum richtig ist
                        // die darstellung auf dem aufgabenblatt ist verwirrend
        }
        //Acknowledge Bit setzen
        header[0] = header[0] | 8;

        //initialisiere Speicher für KEY  (NULLTERM Wird später gesetzt)
        key_len = (uint16_t) header[3] | ((uint16_t)header[2] );    // calculate 'key_len'
        key_buffer = malloc(key_len + 1);       // reserve storage for 'key_buffer'


        printf("key_len ist: %ld\n", key_len);

        //initialisiere Speicher für KEY  (NULLTERM Wird später gesetzt)
        value_len = (uint16_t) header[5] | ((uint16_t)header[4]);  // calculate 'value_len'
        value_buffer = malloc(value_len+ 1);    // reserve storage for 'value_buffer'
        printf("value_len ist: %ld\n", value_len);

        // Receive KEY Byteweise
        while(bytes_recv_all < key_len) {
            if((bytes_recv = recv(their_addr_id, &key_buffer[bytes_recv_all], 1, 0)) == -1) {
                perror("Receive failed");
                continue;
            } else if (bytes_recv == 0) {
                perror("Receive failed");
                continue;
            } else {
                bytes_recv_all += bytes_recv;   // count amount of bytes received in this while-loop in 'bytes_recv_all'
            }
        }
        bytes_recv_all = 0; // Für die nächste Schleife
        // Setzte Nulltermination für key
        key_buffer[key_len] = '\0';


        // RECEIVE BUFFER Byte für Byte
                while(bytes_recv_all < value_len) {
                        if((bytes_recv = recv(their_addr_id, &value_buffer[bytes_recv_all], 1, 0)) == -1) {
                            perror("Receive failed");
                            continue;
                        } else if (bytes_recv == 0) {
                            perror("Receive failed");
                            continue;
                        } else {
                            bytes_recv_all += bytes_recv;   // count amount of bytes received in this hile-loop in 'bytes_recv_all'
                       }
                }   bytes_recv_all = 0; // Für die nächste Schleife
                    //Setze Nulltermination für value
                    value_buffer[value_len] = '\0';


         // Temp Hashnode
        struct hashnode *s = NULL;
        //führe je nach Flag Set, get oder delete aus
        switch(flag){
            //DELETE
            case 1:

                for(int i = 2; i <= 5; i++ )
                {
                    header[i] = 0;              //we dont want key or value len in header in answer
                }
                if ((s = find_movie(key_buffer)) == NULL) // Suche Movie, wenn nicht vorhanden break,
                    {
                        perror("nicht in hash gefunden\n");
                        header[0]= header[0] & 1;


                         // Tarik Version       //set ack bit to 0
                        break;
                    }
                delete_movie(s); // Lösche

                print_movies();
                printf("deleted\n");
                break;

            //SET
            case 2:

                for(int i = 2; i <= 5; i++ )
                {
                    header[i] = 0;              //we dont want key or value len in header in answer
                }
                //Add movie in die Hashmap
                s = find_movie(key_buffer);
                if(( s !=NULL))
                {
                    delete_movie(s);
                }

                add_movie(key_buffer, value_buffer, key_len+1, value_len+1); // Keylen plus 1 für \0

                s = find_movie(key_buffer);
                if(( s == NULL))
                {
                    perror("Fehler beim einfügen in HH\n");
                        header[0]= header[0] & 2;

                }


                print_movies();
                break;

            //GET Sucht
            case 4:
                // Prüft ob Movie vorhanden, wenn es die movie nicht gibt was geben wir zurück, breaken wir
                if ((s = find_movie(key_buffer)) == NULL)
                    {
                        perror("nicht in hash gefunden");
                        header[0] = header[0] & 4; //// Tarik Version      //set ack bit to 0
                        header[4] = 0 ; //
                        header[5] = 0 ;// ich denke das ist sauberer


                        //exit(1);
                        break;
                    }
                //Aktualisiere value len im header, ohne !!!NULLTERM!!!!
                printf("not breaked\n");
                printf("%s\n",s->value);
                printf("%s\n",s->key);

                //header[5] = sizeof(s) - 1;      //asign length of hashed value to answer value_len (-1 because of null-terminierung)
                header[4] = (uint8_t) ((s->value_len-1) >> 8);
                header[5] = (uint8_t) (s->value_len-1);
                printf("new_Value_LenI: %d\n",header[4]);
                printf("new_Value_LenI: %d\n",header[5]);

                break;
            default:
                perror("Sorry kein FLAG");
                break;
        }



/*_________________________________________________|||||||||______________________________________________

         this part is just to test wether my thesis was correct; it is correct: for some reason the client reads header[2] as Key_len LSB
        and header[4] as value_len LSB even though the protocoll is reversed.                                */
        uint8_t tmp = header[2];
        header[2] = header[3];
        header[3] = tmp;

        tmp = header[4];
        header[4] = header[5];
        header[5] = tmp;




        // Sendet den Header

        for (int i = 0; i < 6; ++i)
        {
            // Sende einzeln jedes Element des header Arrays zurück
            printf("SEND: header %d = %d\n",i,header[i]);
             if((b_send = send(their_addr_id, &header[i] , 1, 0))==-1){
                  perror("Sending Failed");
                  continue;
                }
        }

        // schicke value zurück falls flag gesetzt und element in hasmap gefunden
        if (flag == 4 )
        {


            if((b_send = send(their_addr_id, key_buffer, key_len, 0))==-1){
                  perror("Sending Failed");
                  close(sockfd);
                  close(their_addr_id);
                  exit(1);


                }

            if(s != NULL){   // nur wenn Value in der HH
                char* new_value_buffer = malloc(s-> value_len);
                strcpy(new_value_buffer, s->value);
                value_len = s->value_len;
                    if((b_send = send(their_addr_id,new_value_buffer , value_len-1, 0))==-1){
                        perror("Sending Failed");
                        close(sockfd);
                        close(their_addr_id);
                        exit(1);
                    }
                free(new_value_buffer);
            }



        }if(key_buffer != NULL){
            free(key_buffer);
        }
        if(value_buffer != NULL){
            free(value_buffer);
        }
        close(their_addr_id);
    }
    delete_all();
    close(sockfd);
   return 0;
}
