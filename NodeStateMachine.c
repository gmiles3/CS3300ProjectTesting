/*
Interface between application and state layer must be established
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>

#define MAX_PAYLOAD_SIZE 1024
#define MAINHUB 0
#define LOCAL_TIMEOUT_LIMIT 5

typedef enum {
    START,
    SEND_REQUEST,
    WAIT_FOR_INFO,
    SEND_DATA,
    WAIT_FOR_PARENT
} State;

typedef struct { 
	signed short source;
	signed int destination;
	size_t payload_length;
	char* payload;
} packet_t;

typedef struct {
    unsigned short source;
    size_t payload_length;
    char payload[MAX_PAYLOAD_SIZE];
} raw_data;

unsigned short maxID = 0;
size_t bytes;
char data[MAX_PAYLOAD_SIZE];
State current_state = START;
unsigned short nodeID;
struct timeval local_timer;

packet_t* createDataMessage(char* data, size_t bytes) {
	packet_t* packet = malloc(sizeof(packet_t));
	if (packet != NULL) {
		packet->payload = data;
		packet->source = nodeID;
		packet->destination = MAINHUB;
		packet->payload_length = bytes;
	}
	return packet;
}

packet_t* createRequestMessage() {
	packet_t* packet = malloc(sizeof(packet_t));
	if (packet != NULL) {
		packet->source = -1;
		packet->destination = MAINHUB;
		packet->payload_length = 0;
	}
	return packet;
}

int localTimeout() {
    struct timeval current_time;
    gettimeofday(&current_time, NULL);
    if((current_time.tv_sec - local_timer.tv_sec)*1000 > LOCAL_TIMEOUT_LIMIT * (maxID + 1)) {
        gettimeofday(&local_timer, NULL);
        printf("LOCAL TIMEOUT\n");
        return 1;
    } else {
        return 0;
    }
}

// packet_t* receive_packet_now() {
//     packet_t* packet = malloc(sizeof(packet_t));
//     if (packet == NULL) {
//         printf("Mem allocation error in dummy recv packet function");
//         exit(-1);
//     }
//     switch(current_state){
//         case START:
//             packet->source = MAINHUB;
//             packet->destination = -1;
//             printf("Received packet from source MAINHUB with destination -1\n");
//             return packet;
//         case WAIT_FOR_INFO:
//             packet->source = MAINHUB;
//             packet->destination = maxID + 1;
//             printf("Received packet from source MAINHUB with destination %d\n", maxID+1);
//             return packet;
//         case WAIT_FOR_PARENT:
//             packet->source = nodeID - 1;
//             printf("Received packet from source %d with unknown destination\n", nodeID-1);
//             return packet;
//         default:
//             printf("defaulted in receive_packet_now\n");
//     }
// }

packet_t* receive_packet_now() {
    packet_t* packet = malloc(sizeof(packet_t));
    if (packet == NULL) {
        printf("Mem allocation error in dummy recv packet function");
        exit(-1);
    }
    switch(current_state){
        case START:
            packet->source = MAINHUB;
            packet->destination = -1;
            printf("Received packet from source MAINHUB with destination -1\n");
            return packet;
        case WAIT_FOR_INFO:
            packet->source = MAINHUB;
            packet->destination = maxID + 1;
            //printf("Received packet from source -1 with destination %d\n", maxID+1);
            return packet;
        case WAIT_FOR_PARENT:
            packet->source = nodeID - 1;
            printf("Received packet from source %d with unknown destination\n", nodeID-1);
            return packet;
        default:
            printf("defaulted in receive_packet_now\n");
    }
}

void send_packet_now(packet_t* toSend) {
    printf("PACKET SENT WITH PAYLOAD:%c\n", toSend->payload);
}

int main(int argc, char** argv){

    while(1){

        switch(current_state){
            case START: 
                //WAIT FOR BEACON
                printf("case: START\n");
                packet_t* packetIn = NULL;
                while(1) {
                	packetIn = receive_packet_now();//NO parameters?
                	if (packetIn != NULL) { //gets a packet
                		if (packetIn->source == MAINHUB && packetIn->destination == -1) { //check if BEACON message
                			current_state = SEND_REQUEST; //go to send a request
                            free(packetIn);
                			break;
                		}
                	}
                }
                break;
            case SEND_REQUEST:
                printf("case: SEND_REQUEST\n");
                packet_t* request = createRequestMessage();//ready request mem
                if (request == NULL) {
                    printf("oh shit, memory could not be allocated!");
                	exit(-1); // couldn't allocate memory
                }
                send_packet_now(request);//implement this function
            	free(request); //free memory of packet we just sent.
            	current_state = WAIT_FOR_INFO;//lets wait to hear back
                break;
            case WAIT_FOR_INFO:
                printf("case: WAIT_FOR_INFO\n");
                packet_t* info_packet = NULL;
                gettimeofday(&local_timer, NULL);
                while (!localTimeout()) {
                   // printf("blocking on wait_for_info...\n");
                    info_packet = receive_packet_now();
                    if (info_packet != NULL) {
                        if (info_packet->source == MAINHUB && info_packet->destination > MAINHUB )
                        {
                            nodeID = info_packet->destination;
                            maxID = nodeID;
                            current_state = SEND_DATA;
                            free(info_packet); //free becase going to next state
                            break;
                        }
                    }
                }
                if (current_state != SEND_DATA) {
                    //current_state = START;
                    exit(-1);
                }
                break;
            case SEND_DATA:
            	printf("SEND_DATA\n");
                char data[MAX_PAYLOAD_SIZE] = "HELLO WORLD";
            	bytes = fread(data, sizeof(char), MAX_PAYLOAD_SIZE, stdin); //assuming we just always drop data and take most recent data
            	packet_t* toSend = createDataMessage(data, sizeof(data));
            	if (toSend == NULL) {
                    printf("oh shit, memory could not be allocated!");
            		exit(-1); 
            	}
            	send_packet_now(toSend);
            	free(toSend);
            	current_state = WAIT_FOR_PARENT;
            	break;
            case WAIT_FOR_PARENT:
                printf("WAIT_FOR_PARENT\n");
                packet_t* dataIn = NULL;
                while (1) {
                    dataIn = receive_packet_now();
                    if (dataIn != NULL) {
                        if (dataIn->source == MAINHUB && dataIn->destination > MAINHUB) {
                            maxID = dataIn->destination;
                        } else if(dataIn->source == nodeID - 1) {
                            current_state = SEND_DATA;
                            free(dataIn);
                            break;
                        }
                    }
                }
            	break;
            default:
                printf("should not see\n"); 
                break;
        }
    }
    return 0;
}