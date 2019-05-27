#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include "network.h"
#include "uthash.h"

int guard(int n, char *err) {
    if (n == -1) {
        perror(err);
        exit(1);
    }
    return n;
}

//int parse_host(const char *string, user *host, char *files_list) {
//    printf("[Host Parser] Input: %s\n", string);
//    if (strcmp(string, "") == 0)
//        return -3;
//
//    char delim[] = ":";
//    char *cpy = malloc(sizeof(char) * strlen(string));
//    strcpy(cpy, string);
//    int round = 0;
//
//    char *ptr = strtok(cpy, delim);
//
//    // struct sockaddr_in *address = malloc(sizeof(struct sockaddr_in));
//    host->address.sin_family = AF_INET;
//
//    while (ptr != NULL) {
//        if (round == 0) {
//            strcpy(host->nickname, ptr);
//            round++;
//        } else if (round == 1) {
//            struct hostent *host_addr = (struct hostent *) gethostbyname(ptr);
//            if (host_addr == NULL) {
//                // printf("IP Parser: No such host!");
//                return -1;
//            }
//            host->address.sin_addr = *((struct in_addr *) host_addr->h_addr_list[0]);
//            round++;
//        } else if (round == 2) {
//            host->address.sin_port = htons((uint16_t) strtol(ptr, (char **) NULL, 10));
//            if (host->address.sin_port == 0)
//                return -2;
//            round++;
//        } else if (round == 3) {
//            if (files_list != NULL) {
//                strcpy(files_list, ptr);
//            }
//            break;
//        }
//        ptr = strtok(NULL, delim);
//    }
//    printf("[Host Parser] Output: %s\n", addr_to_str(host->address));
//    if (strcmp(addr_to_str(host->address), "0.0.0.0:0") == 0)
//        return -3;
//    return 0;
//}

int parse_ip_port(char *string, struct sockaddr_in *address) {
    char delim[] = ":";
    char *cpy = malloc(sizeof(char) * 21);
    strcpy(cpy, string);
    int round = 0;

    char *ptr = strtok(cpy, delim);

    // struct sockaddr_in *address = malloc(sizeof(struct sockaddr_in));
    address->sin_family = AF_INET;

    while (ptr != NULL) {
        if (round == 0) {
            struct hostent *host = (struct hostent *) gethostbyname(ptr);
            if (host == NULL) {
                // printf("IP Parser: No such host!");
                return -1;
            }
            address->sin_addr = *((struct in_addr *) host->h_addr_list[0]);
            round++;
        } else {
            address->sin_port = htons((uint16_t) strtol(ptr, (char **) NULL, 10));
            if (address->sin_port == 0)
                return -2;
            break;
        }
        ptr = strtok(NULL, delim);
    }
    return 0;
}

int ping(struct sockaddr_in *dest, int close_conn, int *sockfd) {
    // int sockfd = 0;

    dest->sin_family = AF_INET;
    *sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (connect(*sockfd, (struct sockaddr *) dest, sizeof(struct sockaddr)) < 0) {
        printf("[Ping] Can't reach %s. Error: %s (%d).\n", addr_to_str(*dest),
               strerror(errno), errno);
        return -2;
    } else {
        if (close_conn) {
            close(*sockfd);
            return 0;
        }
    }
    return 0;
}

char *addr_to_str(struct sockaddr_in addr) {
    char *tmp = malloc(sizeof(char) * DEFAULT_ADDR_STR_LENGTH);
    sprintf(tmp, "%s:%d", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
    return tmp;
}