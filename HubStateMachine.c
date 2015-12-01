/*
FIX ME'S
MAXID NEEDS TO BE FIXED IN THE EVENT THAT WHEN WE INCREMENT IT THE JOINING NODE DOESN'T ACTUALLY JOIN
PUT MAXID BACK
Talk to WIFI team about packet structure
Talk to WIFI team about send_packet_now sig (initialize function returns an object that should be stored and passed back into the send_packet)
Talk to Application team about multiplexing of payloads / top & middle layer integration

*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <string.h>

#define NODE_LIMIT 5
#define MAX_PAYLOAD_SIZE 1024
#define MAINHUB 0
#define CYCLES_TO_BEACON 50
#define GLOBAL_TIMEOUT_LIMIT 10000
#define LOCAL_TIMEOUT_LIMIT 5
#define INIT_TIMEOUT_LIMIT 2500

typedef enum {
    START,
    SEND_BEACON,
    SEND_PING,
    WAIT_FOR_REQUEST,
    SEND_INFO,
    WAIT_FOR_PARENT
} State;

typedef struct { 
	signed short source;
	signed int destination;
	size_t payload_length;
	char payload[MAX_PAYLOAD_SIZE];
} packet_t;

// typedef struct {
//     unsigned short source;
//     size_t payload_length;
//     char payload[MAX_PAYLOAD_SIZE];
// } raw_data;

unsigned short maxID = 0;
unsigned short beaconCycles = 0;
State current_state = START;
struct timeval global_timer;
struct timeval local_timer;
struct timeval init_timer;
packet_t* globalpipe;
// gettimeofday(&global_timer, NULL);

void createBeaconMessage() {
    globalpipe->source = MAINHUB;
    globalpipe->destination = -1;
    globalpipe->payload_length = 0;
}

void createPingMessage() {
    globalpipe->source = MAINHUB;
    globalpipe->destination = MAINHUB;
    globalpipe->payload_length = 0;
}

void createInfoMessage() {
    globalpipe->source = MAINHUB;
    globalpipe->destination = maxID;
    globalpipe->payload_length = 0;
}

void createDataMessage(char* data, size_t bytes) {
    globalpipe->payload = data;
    globalpipe->source = pthread_self();
    globalpipe->destination = MAINHUB;
    globalpipe->payload_length = bytes;
}

void createRequestMessage() {
    globalpipe->source = -1;
    globalpipe->destination = MAINHUB;
    globalpipe->payload_length = 0;
}

// raw_data* formatVideoData(packet_t* packet) {
//     raw_data* data = malloc(sizeof(raw_data));
//     if (data != NULL) {
//         data->source = packet->source;
//         data->payload_length = packet->payload_length;
//         data->payload = packet->payload;
//     }
//     return data;
// }

int globalTimeout() {
	struct timeval current_time;
	gettimeofday(&current_time, NULL);
	if((current_time.tv_sec - global_timer.tv_sec)*1000 > GLOBAL_TIMEOUT_LIMIT) {
		gettimeofday(&global_timer, NULL);
		return 1;
	} else {
		return 0;
	}
}

int localTimeout() {
    struct timeval current_time;
    gettimeofday(&current_time, NULL);
    if((current_time.tv_sec - local_timer.tv_sec)*1000 > LOCAL_TIMEOUT_LIMIT * (maxID + 1)) {
        gettimeofday(&local_timer, NULL);
        return 1;
    } else {
        return 0;
    }
}

int initTimeout() {
    struct timeval current_time;
    gettimeofday(&current_time, NULL);
    if((current_time.tv_sec - init_timer.tv_sec)*1000 > INIT_TIMEOUT_LIMIT) {
        gettimeofday(&init_timer, NULL);
        return 1;
    } else {
        return 0;
    }
}

packet_t* receive_packet_now() {
    return globalpipe;
}

void send_packet_now() {
    //do nothing..
    printf("Packet Sent\n");
    return;
}

void* node_main() {

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
                            break;
                        }
                    }
                }
                break;
            case SEND_REQUEST:
                printf("case: SEND_REQUEST\n");
                createRequestMessage();//ready request mem
                send_packet_now();//implement this function
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

                            maxID = pthread_self();
                            current_state = SEND_DATA;
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
                // send random data
                createDataMessage(data, sizeof(data));
                send_packet_now();
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
                        } else if(dataIn->source == pthread_self() - 1) {
                            current_state = SEND_DATA;
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


int main(int argc, char** argv){

    gettimeofday(&init_timer,NULL);
    for (int i = 0; i < 4; i++) {
        while(!initTimeout());
        pthread_create(&threads[i], NULL, node_main, NULL);
        fprintf("Thread #%d created", i);
    }

    globalpipe = malloc(sizeof(packet_t));

    pthread_t threads[4];
    fgets
    pthread_create(&threads[i], NULL, node_main, NULL);
    fprintf("Thread #%d created", i);

    while(1){
        switch(current_state){
            case START:
                printf("case: START\n");
                gettimeofday(&global_timer, NULL);
                current_state = SEND_BEACON;
                break;
            case SEND_BEACON:
                printf("case: SEND_BEACON\n");
                beaconCycles++;
                if (beaconCycles == CYCLES_TO_BEACON || maxID == 0) {
                    packet_t* beacon = createBeaconMessage();
                    if (beacon == NULL) {
                        printf("oh shit, memory could not be allocated!\n");
                        exit(-1);
                    }
                    send_packet_now(beacon);
                    beaconCycles = 0;
                    free(beacon);
                    current_state = WAIT_FOR_REQUEST;
                } else {
                    current_state = SEND_PING;
                }
                break;
            case SEND_PING:
                printf("case: SEND_PING\n");
                packet_t* ping = createPingMessage();
                if (ping == NULL) {
                    printf("oh shit, memory could not be allocated!\n");
                    exit(-1);
                }
                send_packet_now(ping);
                free(ping);
                current_state = WAIT_FOR_PARENT;
                break;
            case WAIT_FOR_REQUEST:
                printf("case: WAIT_FOR_REQUEST\n");
                packet_t* requestDataIn = NULL;
                //current_state = SEND_PING; //pre-set state in case of local timeout
                gettimeofday(&local_timer, NULL);
                while (!localTimeout()) {
                    requestDataIn = receive_packet_now();
                    if (requestDataIn != NULL) {
                        if (requestDataIn->source == -1 && requestDataIn->destination == MAINHUB) { //is a request packet
                            current_state = SEND_INFO;
                            free(requestDataIn);
                            break;
                        }
                    }
                }
                if (current_state != SEND_INFO) {
                    if (maxID == 0) {
                        current_state = SEND_BEACON;
                    } else {
                        current_state = SEND_PING;
                    }
                }
                break;
            case SEND_INFO:
            	printf("SEND_INFO\n");
                maxID = maxID + 1;
		        packet_t* info = createInfoMessage();
                if (info == NULL) {
                    printf("oh shit, memory could not be allocated!\n");
                    exit(-1);
                }
		        send_packet_now(info);
                free(info);
                char string[100];
                sprintf(string, "fifo%d.txt", maxID);
                FILE* temp = fopen(string, "wb");
                if (maxID == 1) {
                    pipe1 = temp;
                } else if (maxID == 2) {
                    pipe2 = temp;
                } else if (maxID == 3) {
                    pipe3 = temp;
                } else if (maxID == 4) {
                    pipe4 = temp;
                }
		        current_state = WAIT_FOR_PARENT;
		        break;
            case WAIT_FOR_PARENT:
            	printf("WAIT_FOR_PARENT\n");
            	packet_t* parentDataIn = NULL;
                //current_state = SEND_PING; //pre-set state in case of global timeout
            	while (!globalTimeout()) {
            		parentDataIn = receive_packet_now();
            		if (parentDataIn != NULL) {
                        if (parentDataIn->source != MAINHUB && parentDataIn->destination == MAINHUB) {//is a data packet
            			     //raw_data* data = formatVideoData(dataIn);
                             // if (data == NULL) {
                             //    printf("oh shit, memory could not be allocated!");
                             //    exit(-1);
                             // }
                            if (parentDataIn->source == 1) {
                                fprintf(pipe1, parentDataIn->payload);
                                fclose(pipe1);
                            } else if (parentDataIn->source == 2) {
                                fprintf(pipe2, parentDataIn->payload);
                                fclose(pipe2);
                            } else if (parentDataIn->source == 3) {
                                fprintf(pipe3, parentDataIn->payload);
                                fclose(pipe3);
                            } else if (parentDataIn->source == 4) {
                                fprintf(pipe4, parentDataIn->payload);
                                fclose(pipe4);
                            }
                             free(parentDataIn);
            			     if (parentDataIn->source == maxID) {
		            		    current_state = SEND_BEACON;
                                break;
            			     }
                        }
            		}
            	}
                if (current_state != SEND_BEACON) {
                    current_state = SEND_PING; //pre-set state in case of global timeout
                }
            	break;
            default:
                printf("should not see\n"); 
                break;
        }
    }
    return 0;
}
