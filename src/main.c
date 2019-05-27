#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <string.h>
#include <time.h>
#include <termios.h>
#include <ifaddrs.h>
#include <arpa/inet.h>

#include "network.h"
#include "client.h"
#include "server.h"

void print_usage() {
    printf("Usage: chat [-buf BUFFER_SIZE] [-sv ADDRESS] [-max MAX_USERS] [-cl ADDRESS] [-nick NICKNAME]\n");
}

void print_help() {
    printf("Usage: chat [OPTION...]\n");
    printf("Running with no options will launch client with default options.\n\n");
    printf("Common options:\n\t-buf BUFFER_SIZE\tMaximum message size. Default - 4096\n\n");
    printf("Server options:\n\t-sv ADDRESS\t\tRun chat server at ADDRESS (IP:PORT)\n"
           "\t-max MAX_USERS\t\tSet the maximum number of users. Default - 1024\n\n");
    printf("Client options (CLI will prompt input of missing options):\n\t-cl ADDRESS\t\tRun chat client and connect to server at ADDRESS (IP:PORT)\n"
           "\t-nick NICKNAME\t\tUser's nickname\n\n");
    printf("Help options:\n\t--help\t\t\tShow this help message\n"
           "\t--usage\t\t\tDisplay brief usage message\n\n");
}

int main(int argc, char **argv) {
    BUFFER_LENGTH = 4096;
    MAX_USERS = 1024;
    int server = 0;
    char *nickname = NULL;
    struct sockaddr_in *sv_address = NULL;

    if (argc > 1) {
        for (int i = 1; i < argc; i++) {
            if (strcmp(argv[i], "--help") == 0) {
                print_usage();
                exit(1);
            }

            if (strcmp(argv[i], "--usage") == 0) {
                print_usage();
                exit(1);
            }

            if (strcmp(argv[i], "-sv") == 0) {
                if (i + 1 < argc) {
                    sv_address = malloc(sizeof(struct sockaddr_in));
                    parse_ip_port(argv[i + 1], sv_address);
                    server = 1;
                } else {
                    print_usage();
                    exit(1);
                }
            }

            if (strcmp(argv[i], "-cl") == 0) {
                if (server) {
                    print_usage();
                    exit(1);
                }
                if (i + 1 < argc) {
                    sv_address = malloc(sizeof(struct sockaddr_in));
                    parse_ip_port(argv[i + 1], sv_address);
                    server = 0;
                }
            }

            if (strcmp(argv[i], "-max") == 0) {
                if (i + 1 < argc)
                    MAX_USERS = (int) strtol(argv[i + 1], NULL, 10);
                else {
                    print_usage();
                    exit(1);
                }
            }

            if (strcmp(argv[i], "-buf") == 0) {
                if (i + 1 < argc)
                    BUFFER_LENGTH = (int) strtol(argv[i + 1], NULL, 10);
                else {
                    print_usage();
                    exit(1);
                }
            }

            if (strcmp(argv[i], "-nick") == 0) {
                if (i + 1 < argc) {
                    nickname = malloc(sizeof(char) * NICKNAME_LENGTH);
                    strcpy(nickname, argv[i + 1]);
                } else {
                    print_usage();
                    exit(1);
                }
            }
        }
    }

    if (server)
        run_server(sv_address);
    else {
        // Client mode
        if (nickname == NULL) {
            char *c = malloc(sizeof(char)); // Used to remove trailing \n
            nickname = malloc(sizeof(char) * DEFAULT_ADDR_STR_LENGTH);
            printf("Your nickname: ");
            scanf("%s%c", nickname, c);
        }

        if (sv_address == NULL) {
            char *sv_address_str = malloc(sizeof(char) * DEFAULT_ADDR_STR_LENGTH);
            memset(sv_address_str, 0, DEFAULT_ADDR_STR_LENGTH);
            sv_address = malloc(sizeof(struct sockaddr_in));
            printf("Server address: ");
            scanf("%s", sv_address_str);
            if (parse_ip_port(sv_address_str, sv_address) != 0) {
                fprintf(stderr, "Address parsing error\n");
                exit(1);
            }
        }
        run_client(sv_address, nickname);
    }
    return 0;
}

