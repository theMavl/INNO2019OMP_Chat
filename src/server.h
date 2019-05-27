#ifndef CHAT_SERVER_H
#define CHAT_SERVER_H

#include <arpa/inet.h>
#include "network.h"

int MAX_USERS;

struct user_hashed {
    int sock_fd;
    int registered;
    char nickname[NICKNAME_LENGTH];
    struct sockaddr_in address;
    UT_hash_handle hh;
};

void run_server(struct sockaddr_in *server_addr);

#endif //CHAT_SERVER_H
