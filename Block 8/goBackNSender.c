/***************************************************************************
 *
 * Copyright:   (C)2004-2018 Telecommunication Networks Group (TKN) at
 *              Technische Universitaet Berlin, Germany.
 *
 * Authors:     Lars Westerhoff, Guenter Schaefer, Daniel Happ
 *
 **************************************************************************/

#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <stdbool.h>
#include <inttypes.h>
#include <limits.h>

#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>

#include <time.h>
#include "timeval_macros.h"

#include "DataBuffer.h"
#include "SocketConnection.h"

#define DEBUG
#ifdef DEBUG
#define DEBUGOUT(...) fprintf(stderr, __VA_ARGS__)
#else
#define DEBUGOUT(...)
#endif

#define DEFAULT_REMOTE_PORT "4343"
#define DEFAULT_PAYLOAD_SIZE 1024
#define MAX_FILE_SIZE 1024

struct timeval timeout;
unsigned window;
char* remotePort;
char* remoteName;
char* fileName;
DataBuffer dataBuffer;

long lastAckSeqNo;
long nextSendSeqNo;

struct timeval timerExpiration;

void help(int exitCode) {
  fprintf(stderr,
          "GoBackNSender [--timeout|-t msec] [--window|-w count] [--remote|-r "
          "port] hostname file\n");

  exit(exitCode);
}

void initialize(int argc, char** argv) {
  timeout.tv_sec = 3;
  timeout.tv_usec = 0;

  timerExpiration.tv_sec = LONG_MAX;
  timerExpiration.tv_usec = 0;

  window = 25;
  remotePort = DEFAULT_REMOTE_PORT;

  while (1) {
    static struct option long_options[] = {{"timeout", 1, NULL, 't'},
                                           {"window", 1, NULL, 'w'},
                                           {"remote", 1, NULL, 'r'},
                                           {"help", 0, NULL, 'h'},
                                           {0, 0, 0, 0}};

    int c = getopt_long(argc, argv, "t:w:r:h", long_options, NULL);
    if (c == -1) break;

    int retval = 0;
    switch (c) {
      case 't': {
        unsigned msec;
        retval = sscanf(optarg, "%u", &msec);
        if (retval < 1) help(1);
        timeout.tv_sec = msec / 1000;
        timeout.tv_usec = (msec % 1000) * 1000;
      } break;

      case 'w':
        retval = sscanf(optarg, "%u", &window);
        if (retval < 1) help(1);
        break;

      case 'r':
        remotePort = optarg;
        break;

      case 'h':
        help(0);
        break;

      case '?':
        help(1);
        break;

      default:
        printf("?? getopt returned character code 0%o ??\n", c);
    }
  }

  if (argc < optind + 2 || window <= 0) help(1);

  remoteName = argv[optind];
  fileName = argv[optind + 1];

  dataBuffer = allocateDataBuffer(MAX_FILE_SIZE);

  lastAckSeqNo = nextSendSeqNo = 0;
}

bool readIntoBuffer(FILE* file, long seqNo) {
  DataPacket* dataPacket = (DataPacket*)malloc(sizeof(DataPacket));

  dataPacket->packet = (GoBackNMessageStruct*)malloc(
      sizeof(GoBackNMessageStruct) + DEFAULT_PAYLOAD_SIZE);
  dataPacket->packet->seqNo = seqNo;
  dataPacket->packet->seqNoExpected = -1;
  dataPacket->packet->crcSum = 0;

  size_t bytesRead =
      fread(dataPacket->packet->data, 1, DEFAULT_PAYLOAD_SIZE, file);
  DEBUGOUT("FILE: %zu bytes read\n", bytesRead);
  dataPacket->packet->size = bytesRead + sizeof(GoBackNMessageStruct);

  dataPacket->packet->crcSum = crcGoBackNMessageStruct(dataPacket->packet);

  if (bytesRead < DEFAULT_PAYLOAD_SIZE) {
    if (ferror(file)) {
      perror("fread");
      exit(1);
    }
    if (bytesRead == 0) {
      putDataPacketIntoBuffer(dataBuffer, dataPacket);
      return false;
    }
  }

  putDataPacketIntoBuffer(dataBuffer, dataPacket);
  return true;
}

long readFileIntoBuffer() {
  long seqNo = 0;

  FILE* input = fopen(fileName, "rb");
  if (input == NULL) {
    perror("fopen");
    exit(1);
  }

  while (readIntoBuffer(input, seqNo)) ++seqNo;
  fclose(input);

  return seqNo;
}

int main(int argc, char** argv) {
  // parse command line arguments
  initialize(argc, argv);

  // read file
  long veryLastSeqNo = readFileIntoBuffer();
  DEBUGOUT("veryLastSeqNo: %ld\n", veryLastSeqNo);

  // Prepare channel to receiver
  // We use "connect()" here because we have only one receiver.
  // Despite using UDP, you will have to use send()/recv() later!
  int s = udp_connect(remoteName, remotePort);
  if (s < 0) {
    exit(1);
  }

  while ( lastAckSeqNo != veryLastSeqNo) {
    DEBUGOUT("nextSendSeqNo: %ld, lastAckSeqNo: %ld\n", nextSendSeqNo, lastAckSeqNo);

    fd_set readfds, writefds;
    FD_ZERO(&readfds);
    FD_ZERO(&writefds);
    FD_SET(s, &readfds);

    if (nextSendSeqNo <= veryLastSeqNo &&
        nextSendSeqNo <= getLastSeqNoOfBuffer(dataBuffer) &&
        nextSendSeqNo < lastAckSeqNo + window) {
      FD_SET(s, &writefds);
    }

    struct timeval selectTimeout, currentTime;
    if (gettimeofday(&currentTime, NULL) < 0) {
      perror("gettimeofday");
      exit(1);
    }
    DEBUGOUT("current time: %ld,%ld\n", currentTime.tv_sec,
             currentTime.tv_usec);

    timersub(&timerExpiration, &currentTime, &selectTimeout);

    if (selectTimeout.tv_sec < 0 || selectTimeout.tv_usec < 0) {
      fprintf(stderr, "WARNING: Timeout was negative (%ld,%ld)\n",
              selectTimeout.tv_sec, selectTimeout.tv_usec);
      selectTimeout.tv_sec = selectTimeout.tv_usec = 0;
    }

    int n;
    if ((n = select(s + 1, &readfds, &writefds, NULL, &selectTimeout)) < 0) {
      perror("select");
      exit(1);
    }

    // Handle acknowledgements
    if (FD_ISSET(s, &readfds)) {
      uint32_t tmpCRC = 0;
      bool crcValid;
      int bytesRead = 0;

      GoBackNMessageStruct* ack = allocateGoBackNMessageStruct(0);
      if ((bytesRead = recv(s, ack, sizeof(*ack), MSG_DONTWAIT)) < 0) {
        perror("recv");
        exit(1);
      }
      DEBUGOUT("SOCKET: %d bytes received\n", bytesRead);

      tmpCRC = ack->crcSum;
      ack->crcSum = 0;
      crcValid = (tmpCRC == crcGoBackNMessageStruct(ack));

      /* YOUR TASK:
       * 
       * 
       * 
       * 
       *   # Still sent but unacknowledged packets in the buffer
       *   # No packets waiting for acknowledgement
       * //- If needed: Set nextSendSeqNo so that never packets are sent
       *              that where already acknowledged
       *
       * FUNCTIONS YOU MAY NEED:
       * - freeBuffer()
       * - getDataPacketFromBuffer()
       */
      if(crcValid){                                                   //- Check acknowledgement for errors (crcValid) 
          if(lastAckSeqNo < ack->seqNoExpected){                    //- Are new packets acknowledged by this packet?
            printf("\n %ld Acknowledgements empfangen. \n\n",  ack->seqNoExpected - lastAckSeqNo);
            freeBuffer(dataBuffer,lastAckSeqNo,ack->seqNoExpected-1); //- Free buffers of acknowledged packets
            lastAckSeqNo = ack->seqNoExpected;                      //- Set lastAckSeqNo as needed
            if( lastAckSeqNo < nextSendSeqNo){                     //- Recalculate timerExpiration, distinguish between two cases:
             // # Still sent but unacknowledged packets in the buffer
              printf("Still sent but unacknowledged packets in the buffer\n" );
              timerExpiration.tv_sec = getDataPacketFromBuffer(dataBuffer,lastAckSeqNo)->timeout.tv_sec;
              timerExpiration.tv_usec = getDataPacketFromBuffer(dataBuffer,lastAckSeqNo)->timeout.tv_usec;  
            }
            else{// ansonsten reset timeout
              resetTimers(dataBuffer);
              nextSendSeqNo=lastAckSeqNo;
              }
            
          }
        }
          
      /* END YOUR TASK */
      freeGoBackNMessageStruct(ack);
    }

    // Handle timeout
    if (gettimeofday(&currentTime, NULL) < 0) {
      perror("gettimeofday");
      exit(1);
    }
    //DEBUGOUT("current time: %ld,%ld\n", currentTime.tv_sec, currentTime.tv_usec);
    //DEBUGOUT("Timeout: %ld,%ld\n", timerExpiration.tv_sec, timerExpiration.tv_usec);
    if (timercmp(&timerExpiration, &currentTime, < )) {
      DEBUGOUT("TIMEOUT (Current: %ld,%ld; expiration: %ld,%ld)\n",
               currentTime.tv_sec, currentTime.tv_usec, timerExpiration.tv_sec,
               timerExpiration.tv_usec);
      /* YOUR TASK:
       * - Make sure that all unacknowledged packets will be resent (do
       *   NOT send them here!)
       * - Reset timers
       *
       * FUNCTIONS YOU MAY NEED:
       * - resetTimers()
       */
      nextSendSeqNo = lastAckSeqNo  ;
      resetTimers(dataBuffer);
      /* END YOUR TASK */
      timeradd(&currentTime, &timeout, &timerExpiration);
    }

    // Send packets

    if (FD_ISSET(s, &writefds)) {

      while (nextSendSeqNo < (lastAckSeqNo  + window) && nextSendSeqNo <= veryLastSeqNo /* YOUR TASK: When are you allowed to send new packets? */) {
        DataPacket* data = getDataPacketFromBuffer(dataBuffer, nextSendSeqNo);

        // Send data
        int retval = send(s, data->packet, data->packet->size, MSG_DONTWAIT);
        if (retval < 0) {
          if (errno == EAGAIN)
            break;
          else {
            perror("send");
            exit(1);
          }
        }

        DEBUGOUT("SOCKET: %d bytes sent\n", retval);
        printf("\nnextSendSeqNo: %ld < veryLastSeqNo %ld \n lastAckSeqNo: %ld\n", nextSendSeqNo,veryLastSeqNo , lastAckSeqNo);
        /* YOUR TASK: Sending was successful, what now?
         * - Update sequence numbers
         */

        /* END YOUR TASK */

        // Update timers
        struct timeval currentTime;
        if (gettimeofday(&currentTime, NULL) < 0) {
          perror("gettimeofday");
          exit(1);
        }
        DEBUGOUT("current time: %ld,%ld\n", currentTime.tv_sec,
                 currentTime.tv_usec);

        /* YOUR TASK:
         * - Store the timeout with the packet
         *
         * MACROS YOU MAY NEED:
         * - timeradd(a, b, result)
         * - timercmp(a, b, cmp) // NOTE: cmp is a comparison operator
         *                          like <, >, <=, >=, ==
         */
         timeradd(&timeout, &currentTime, &(getDataPacketFromBuffer(dataBuffer,nextSendSeqNo)->timeout));
        nextSendSeqNo ++;

         //Recalculate timerExpiration
         timerExpiration.tv_sec = getDataPacketFromBuffer(dataBuffer,lastAckSeqNo)->timeout.tv_sec;
         timerExpiration.tv_usec = getDataPacketFromBuffer(dataBuffer,lastAckSeqNo)->timeout.tv_usec; 
      }
    }
  }
  close(s);
  deallocateDataBuffer(dataBuffer);
  return 0;
}
