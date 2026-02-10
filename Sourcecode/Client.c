//Name: Dhanasekar Ravi Jayanthi
//SCU ID: 07700005982


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <poll.h>
#include <unistd.h>
#include <arpa/inet.h>

#define MAX_PACKETS 5
#define START_IDENTIFIER 0xAAAA
#define END_IDENTIFIER 0xBBBB
#define CLIENT_IDENTIFIER 0x99
#define PACKET_TYPE_DATA 0xCCCC
#define PACKET_TYPE_ACK 0xDDDD
#define PACKET_TYPE_REJECT 0xEEEE
#define REJECT_SEQUENCE 0x1111
#define REJECT_SIZE 0x2222
#define REJECT_END 0x3333
#define REJECT_DUPLICATE 0x4444

#define SERVER_PORT 54321
#define BUFFER_SIZE 2048
#define TIMEOUT_MS 3000  // Timeout in milliseconds

// Struct Packing for Network Efficiency
#pragma pack(1)
typedef struct {
    unsigned short start_id;
    unsigned char client_id;
    unsigned short packet_type;
    char packet_number;
    char data_length;
    char message[255];
    unsigned short end_id;
} DataPacket;

typedef struct {
    unsigned short start_id;
    unsigned char client_id;
    unsigned short packet_type;
    unsigned short reject_code;
    unsigned char received_packet;
    unsigned short end_id;
} ServerResponse;
#pragma pack()

void prepare_packets(DataPacket packets[], int test_case);
void handle_server_response(int sock, struct sockaddr_in *server, socklen_t addr_len, DataPacket *packet, int *current_packet);
//test case selection
int main() {
    int test_case;
    printf("\n Select a Test Case:\n");
    printf("1. Normal Transmission\n2. Sequence Error\n3. Size Error\n4. End Marker Error\n5. Duplicate Packet\n");
    printf("Enter choice: ");
    scanf("%d", &test_case);
// server address setup
    struct sockaddr_in server_addr;
    int sock, i, addr_len = sizeof(server_addr);
    char *server_ip = "127.0.0.1";
// UDP socket creation
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Error: Failed to create socket");
        return -1;
    }
//port number and server addres to coonect to server
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);

    if (inet_aton(server_ip, &server_addr.sin_addr) == 0) {
        fprintf(stderr, "Error: Invalid server address\n");
        close(sock);
        return -1;
    }
// Array of 5 packets
    DataPacket packets[MAX_PACKETS];
    prepare_packets(packets, test_case);

    for (i = 0; i < MAX_PACKETS; i++) {
        handle_server_response(sock, &server_addr, addr_len, &packets[i], &i);
    }

    close(sock);
    printf("\nAll packets sent and processed successfully.\n");
    return 0;
}
//Creating packet
void prepare_packets(DataPacket packets[], int test_case) {
    for (int i = 0; i < MAX_PACKETS; i++) {
        packets[i].start_id = START_IDENTIFIER;
        packets[i].client_id = CLIENT_IDENTIFIER;
        packets[i].packet_type = PACKET_TYPE_DATA;
        packets[i].packet_number = i;
        sprintf(packets[i].message, "Packet %d Data", i);
        packets[i].data_length = strlen(packets[i].message) + 1;
        packets[i].end_id = END_IDENTIFIER;
    }
//Error handling
    switch (test_case) {
        case 2:  // Sequence Error
            packets[3].packet_number = 4;
            packets[4].packet_number = 3;
            break;
        case 3:  // Size Error
            packets[2].data_length = 3;
            break;
        case 4:  // End Marker Error
            packets[2].end_id = 0xDEAD;
            break;
        case 5:  // Duplicate Packet
            packets[2] = packets[1];
            break;
        default:
            break;
    }
}
//response from seerver
void handle_server_response(int sock, struct sockaddr_in *server, socklen_t addr_len, DataPacket *packet, int *current_packet) {
    int retries = 0;
    ServerResponse response;
//retransmission
    while (retries < 3) {
        printf("\nSending Packet %d (Attempt %d)\n", packet->packet_number, retries + 1);
//packet sending
        if (sendto(sock, packet, sizeof(DataPacket), 0, (struct sockaddr *)server, addr_len) < 0) {
            perror("Error: Failed to send packet");
            return;
        }
// ack timer
        struct pollfd fd;
        fd.fd = sock;
        fd.events = POLLIN;

        int poll_result = poll(&fd, 1, TIMEOUT_MS);

        if (poll_result > 0) {
            if (recvfrom(sock, &response, sizeof(ServerResponse), 0, (struct sockaddr *)server, &addr_len) < 0) {
                perror("Error: Failed to receive response");
                return;
            }

            if (response.packet_type == PACKET_TYPE_ACK) {
                printf("ACK received for Packet %d\n", packet->packet_number);
                (*current_packet)++;
                return;
            } else if (response.packet_type == PACKET_TYPE_REJECT) {
                printf("REJECT received: ");
                switch (response.reject_code) {
                    case REJECT_SEQUENCE:
                        printf("Sequence Error: Expected %d, received %d.\n", *current_packet, response.received_packet);
                        break;
                    case REJECT_SIZE:
                        printf("Size Error: Packet %d has incorrect data length.\n", *current_packet);
                        break;
                    case REJECT_END:
                        printf("End Marker Error: Packet %d is malformed.\n", *current_packet);
                        break;
                    case REJECT_DUPLICATE:
                        printf("Duplicate Error: Packet %d was already received.\n", response.received_packet);
                        break;
                    default:
                        printf("Unknown Error.\n");
                        break;
                }
                return;
            }
        } else if (poll_result == 0) {
            printf("Timeout: Retrying Packet %d...\n", packet->packet_number);
        } else {
            perror("Polling Error");
            return;
        }

        retries++;
    }

    printf("No response from server after 3 attempts. Moving to next packet.\n");
}
