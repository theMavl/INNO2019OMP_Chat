#ifndef CHAT_NETWORK_H
#define CHAT_NETWORK_H

#include <arpa/inet.h>
#include "uthash.h"

#define DEFAULT_ADDR_STR_LENGTH 21
#define NICKNAME_LENGTH 32

#define CL_REG 0 // Client registration request (set nickname)
#define CL_MSG 1 // Message from client
#define BC_MSG 2 // Server broadcasts client message
#define SV_MSG 3 // Message from server

int BUFFER_LENGTH;

typedef struct _message_header {
    int type;
    int length;
} message_header;

int guard(int n, char *err);

int parse_ip_port(char *string, struct sockaddr_in *result);

int ping(struct sockaddr_in *dest, int close_conn, int *sockfd);

char *addr_to_str(struct sockaddr_in addr);

#endif //CHAT_NETWORK_H
