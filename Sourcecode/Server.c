//Name: Dhanasekar Ravi Jayanthi
//SCU ID: 07700005982



#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

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
//UDP socket creation to recive data packets
void send_ack(int sock, struct sockaddr_in *client, socklen_t addr_len, char packet_number);
void send_reject(int sock, struct sockaddr_in *client, socklen_t addr_len, unsigned short reject_code, char packet_number);
// socket creation
int main() {
    int sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    DataPacket packet;
    int expected_packet = 0;

    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Error: Failed to create socket");
        return -1;
    }
// server address setup
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(SERVER_PORT);
//Socket binding
    if (bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error: Failed to bind socket");
        close(sock);
        return -1;
    }

    printf("Server is Running... Listening for packets on port %d...\n", SERVER_PORT);
//Packet Handling loop
    while (1) {
        if (recvfrom(sock, &packet, sizeof(packet), 0, (struct sockaddr *)&client_addr, &addr_len) < 0) {
            perror("Error: Failed to receive packet");
            continue;
        }

        printf("\nReceived Packet #%d\n", packet.packet_number);

        // Packet Integrity Checks
        if (packet.start_id != START_IDENTIFIER || packet.end_id != END_IDENTIFIER) {
            printf("Error: Packet Format Invalid. Sending REJECT (End Marker Error).\n");
            send_reject(sock, &client_addr, addr_len, REJECT_END, packet.packet_number);
            continue;
        }

        if (packet.client_id != CLIENT_IDENTIFIER) {
            printf("Warning: Unknown Client ID. Ignoring packet.\n");
            continue;
        }

        if (packet.packet_type != PACKET_TYPE_DATA) {
            printf("Warning: Unexpected Packet Type. Ignoring packet.\n");
            continue;
        }

        // Handling Sequence Errors order of packets and dupicate
        if (packet.packet_number != expected_packet) {
            if (packet.packet_number < expected_packet) {
                printf("Error: Duplicate Packet Detected. Sending REJECT (Duplicate Error).\n");
                send_reject(sock, &client_addr, addr_len, REJECT_DUPLICATE, packet.packet_number);
            } else {
                printf("Error: Out-of-Sequence Packet. Sending REJECT (Sequence Error).\n");
                send_reject(sock, &client_addr, addr_len, REJECT_SEQUENCE, packet.packet_number);
            }
            continue;
        }

        // Data Size Verification
        if (packet.data_length != strlen(packet.message) + 1) {
            printf("Error: Data Size Mismatch. Sending REJECT (Size Error).\n");
            send_reject(sock, &client_addr, addr_len, REJECT_SIZE, packet.packet_number);
            continue;
        }

        // If all checks passed, send ACK
        printf("Packet %d is valid. Sending ACK.\n", packet.packet_number);
        send_ack(sock, &client_addr, addr_len, packet.packet_number);
        expected_packet++;
    }

    close(sock);
    return 0;
}
// response function 
//Acknowledgment function
void send_ack(int sock, struct sockaddr_in *client, socklen_t addr_len, char packet_number) {
    ServerResponse ack = {
        .start_id = START_IDENTIFIER,
        .client_id = CLIENT_IDENTIFIER,
        .packet_type = PACKET_TYPE_ACK,
        .reject_code = 0,
        .received_packet = packet_number,
        .end_id = END_IDENTIFIER
    };

    if (sendto(sock, &ack, sizeof(ack), 0, (struct sockaddr *)client, addr_len) < 0) {
        perror("Error: Failed to send ACK");
    } else {
        printf("ACK Sent for Packet %d\n", packet_number);
    }
}
//Rejection Function
void send_reject(int sock, struct sockaddr_in *client, socklen_t addr_len, unsigned short reject_code, char packet_number) {
    ServerResponse reject = {
        .start_id = START_IDENTIFIER,
        .client_id = CLIENT_IDENTIFIER,
        .packet_type = PACKET_TYPE_REJECT,
        .reject_code = reject_code,
        .received_packet = packet_number,
        .end_id = END_IDENTIFIER
    };

    if (sendto(sock, &reject, sizeof(reject), 0, (struct sockaddr *)client, addr_len) < 0) {
        perror("Error: Failed to send REJECT");
    } else {
        printf("REJECT Sent for Packet %d with Code 0x%X\n", packet_number, reject_code);
    }
}
