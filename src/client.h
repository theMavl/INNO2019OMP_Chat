#ifndef CHAT_CLIENT_H
#define CHAT_CLIENT_H

#include <arpa/inet.h>

int run_client(struct sockaddr_in *server_address, char *nickname);

#endif //CHAT_CLIENT_H
