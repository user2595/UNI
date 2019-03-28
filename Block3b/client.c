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
#include <sys/time.h>   
#include <sys/resource.h> 


// ________________________________________  FUNCTIONS START  ________________________________________





// get an ip_address and a port_no -> create a socket, return an integer socket_id
int connect_to_server (char* ip_address, char* port_no) {
    struct addrinfo hints, *server_address, *ptr;
    int sock_id;

// set 'hints'
    memset(&hints, 0, sizeof (struct addrinfo));    // set memory to 0
    hints.ai_family = AF_UNSPEC;                    // allow ip_v4 and ip_v6
    hints.ai_socktype = SOCK_STREAM;                // Datastream(TCP)

// execute 'getaddrinfo' -> fill in 'server_address' addrinfo using adress, portno., 'hints'   
    if (getaddrinfo(ip_address, port_no, &hints, &server_address) != 0) {
        perror("Getaddrinfo failed\n"); 
        return -1;
    }

// loop : create a socket
    for(ptr = server_address ; ptr != NULL ; ptr = ptr->ai_next) {
    // execute 'socket' -> create socket 'sock_id' 
        if ((sock_id = socket(server_address->ai_family, server_address->ai_socktype, server_address->ai_protocol))== -1) {
            perror("Socket failed\n");
            continue;
        }
        server_address = ptr;
        break;
    }

// execute 'connect' -> assign 'sock_id' the server ip_address stored in 'server_address->ai_addr'
    if (connect(sock_id, server_address->ai_addr, server_address->ai_addrlen) == -1) {
        perror("Connect failed\n");
        close(sock_id);
        return -1;
    }

    freeaddrinfo(server_address);

// return 'socket_id' or -1 of failure 
    return sock_id;
}


// get a byte amount, an socket id and a pointer to a storage -> store the amount of bytes from 'recv' into the storage, return -1 on failure
int receive_bytes (int recv_amount, int socket, uint8_t *storage) {   
	int bytes_recv = 0;
    int bytes_recv_all = 0;					
// loop : receive the full amount of bytes, one by one
    while(bytes_recv_all < recv_amount) {
        if((bytes_recv = recv(socket, &storage[bytes_recv_all], 1, 0)) == -1) {
        	perror("Receive failed -1");
        	close(socket);
        	return -1;
        } else if (bytes_recv == 0) {
        	continue;
        } else {
        	bytes_recv_all += bytes_recv;	// count bytes received in 'bytes_recv_all'
            bytes_recv = 0;                 // redundant for safety
        }
    }
    return 0;
}


// #ERROR : STORAGE TYPE (UNSIGNED INT / CHAR), probably not though
int send_bytes (int send_amount, int socket, uint8_t *storage) {
    int bytes_send = 0;
    int bytes_send_all = 0;
// loop : send the full amount of bytes, one by one
    while(bytes_send_all < send_amount) {
    	//printf("send[%d]: %d\n",bytes_send_all, storage[bytes_send_all]); // #DEBUG
        if((bytes_send = send(socket, &storage[bytes_send_all], 1, 0)) == -1) {
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


// ________________________________________  REQUEST START  ________________________________________


// #TODO: RETURN TYPE
int request_function(int socket_id, uint8_t flag, char *key, char *value) {
	uint8_t header_buffer[6];
	char *key_buffer, *value_buffer, *key_answ_buffer, *value_answ_buffer;
	uint16_t key_len = strlen(key);
	//printf("Key_len: %d\n", key_len); // #DEBUG
	uint16_t value_len;
	struct timeval timer;	// timeout timer used in 'select'
	timer.tv_sec = 2;
	timer.tv_usec = 0;
	fd_set socket_read_set;
	int select_return;


// initialize header
	header_buffer[0] = flag;
	header_buffer[1] = 0;
	header_buffer[2] = (uint8_t) (key_len >> 8);	// set key_len MSB
	header_buffer[3] = (uint8_t) (key_len);		// set key_len LSB
	header_buffer[4] = 0;
	header_buffer[5] = 0;


// set key in 'key_buffer'
	key_buffer = malloc((key_len+1) * sizeof(char));
	strcpy(key_buffer, key);


// additionally set value in 'value_buffer' and value_len
	if(flag == 2) {
		value_len = strlen(value);
		//printf("Value_len: %d\n", value_len); // #DEBUG
		value_buffer = malloc((value_len + 1) * sizeof(char));
		strcpy(value_buffer, value);
		header_buffer[4] = (uint8_t) (value_len >> 8);	// set value_len MSB
		header_buffer[5] = (uint8_t) (value_len);		// set value_len LSB
	}


// loop : send request and repeat until we receive a good answer
	while(1) {
	
	//printf("start While\n"); // #DEBUG	

	// set socket_id to use in 'select'
		FD_ZERO(&socket_read_set);
		FD_SET(socket_id, &socket_read_set);

	// send everything
		// #DEBUG
		//printf("HEADER: %d %d %d %d %d %d\n", header_buffer[0], header_buffer[1], header_buffer[2], header_buffer[3], header_buffer[4], header_buffer[5]); // #DEBUG


// ________________________________________  SEND START  ________________________________________


		if(send_bytes(6, socket_id, header_buffer) == -1) { // send header
			perror("Send_Header failed\n");
			return -1;
		}

		if(send_bytes(key_len, socket_id, (uint8_t * )key_buffer) == -1) {	// send key
			perror("Send_Key failed\n");
			return -1;
		}

		if(flag == 2) {
			if(send_bytes(value_len, socket_id, (uint8_t * )value_buffer) == -1) {	// send value if 'flag' is GET
				perror("Send_Value failed\n");
				return -1;
			}
		}


	// timer
		if((select_return = select(socket_id+1, &socket_read_set, NULL, NULL, &timer)) <= 0) {	// check both error cases
			perror("Timeout \n"); // #DEBUG
			continue;
		}


// ________________________________________  RECEIVE START  ________________________________________


		//printf("start recv Header\n"); // #DEBUG
	// receive header
		if(receive_bytes(6, socket_id, header_buffer) == -1) {
			perror("Receive_Header failed\n"); // #DEBUG	
			continue;
		}

		//printf("RECEIVED HEADER: %d %d %d %d %d %d\n", header_buffer[0], header_buffer[1], header_buffer[2], header_buffer[3], header_buffer[4], header_buffer[5]); // #DEBUG

	// prepare 'key_buffer' and 'value_buffer'
		key_answ_buffer = malloc((key_len+1) * sizeof(char));
		value_len = (uint16_t) header_buffer[5] + ((uint16_t)header_buffer[4] << 8);
		value_answ_buffer = malloc((value_len + 1) * sizeof(char));

		//printf("RECEIVED Value_len: %d\n", value_len);	// #DEBUG

	// errorchecking for the case a bad key/value len gets returned.... Jonas.. Tarik..
		if(flag != 4) {
			key_len = 0;
			value_len = 0;
		}

		//printf("start recv Key\n"); // #DEBUG
	// receive key
		if(receive_bytes(key_len, socket_id, (uint8_t * )key_answ_buffer) == -1) {	// #ERROR : receive waits until connection closes, timeout missing
			perror("Receive_Key failed\n"); // #DEBUG	
			continue;
		}

		//printf("start recv Value\n"); // #DEBUG
	// receive value
		if(receive_bytes(value_len, socket_id, (uint8_t * )value_answ_buffer) == -1 && flag == 2) {	// redundant for safety
			perror("Receive_Value failed\n"); // #DEBUG	
			continue;
		}

		//printf("start Return\n"); // #DEBUG
	// receive was successful
		if(flag == 2) {		
			//printf("Flag[%d]: free valuebuffer\n", flag); // #DEBUG
			free(value_buffer);
		} 
		else if(flag == 4) {
			//printf("Flag[%d]: Try printing Result\n", flag); // #DEBUG
			printf("Result: %s\n", value_answ_buffer); // #ERROR : print without \0 needed
			free(value_answ_buffer);
		}
		//printf("Flag[%d]: free keybuffer\n", flag); // #DEBUG
		free(key_buffer);
		free(key_answ_buffer);

		
		FD_ZERO(&socket_read_set);

	// 'request_function' successfully send and received.
		return 0;	// #TODO : 'Return value_answ_buffer' instead of print 

	}

}

// ________________________________________  MAIN START  ________________________________________



int main(int argc, char *argv[]) {
    int socket_id;
    uint8_t flag;	// flag that stores the request type
    char *key = argv[4];	// #ERROR : Maybe strings from argv[] dont end with \0
    char *value = NULL;

// errorcheck : argument count
    if (argc < 5 || argc > 6) {
        perror("Usage: ip_address, port_no, request_type, key, (value)\n");
        return -1;
    }
    

// execute 'connect_to_server' -> create a socket accessable by 'socket_id'
    if((socket_id = connect_to_server(argv[1], argv[2])) == -1) {
        perror("Connect to Server failed\n");
        return -1;
    }




// ________________________________________  PROGRAMM START  ________________________________________



// DELETE Case
    if(strcmp(argv[3],"DELETE") == 0) {
    	flag = 1;
    } 

// SET Case
    else if(strcmp(argv[3],"SET") == 0) {
    // errorcheck
    	if(argc < 6) {
    		perror("Usage: ip_address, port_no, SET, key, value\n");
        	return -1;
    	}
    	flag = 2;
    	value = argv[5];
    } 

// GET Case
    else if(strcmp(argv[3],"GET") == 0) {
    	flag = 4;
    }

// No Flag
    else {
    	perror("Usage: ip_address, port_no, request_type, key, (value)\n");
        return -1;
    }

// Execute Request Function, Value is NULL for flag = 1 or flag = 4
    //printf("Key: %s\nValue: %s\n", key, value); // #DEBUG
    request_function(socket_id, flag, key, value);	


// free and close everything
	
	close(socket_id);


    return 0;
}
 
