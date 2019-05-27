#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <memory.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <termios.h>

#include "network.h"
#include "uthash.h"
#include "client.h"
#include "tty_utils.h"


char *local_buffer;
int buffer_ptr = 0;
int line_feed = 0;
int termination = 0;

void print_sv_msg(char *message) {
    struct winsize w;

    if (line_feed) {
        printf("\x1b[A\x1b[A");
        line_feed = 0;
    }

    printf("\r\033[3m%s\033[0m", message);

    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    for (int i = strlen(message); i < w.ws_col; i++) {
        putchar(' ');
    }

    printf("\n\n> %s", local_buffer);
    fflush(stdout);
}

void print_bc_msg(char *nickname, char *message) {
    struct winsize w;
    char from_str[70];
    time_t t = time(NULL);

    char datetime_str[32];
    struct tm datetime;

    (void) localtime_r(&t, &datetime);
    strftime(datetime_str, sizeof(datetime), "%H:%M", &datetime);
    if (line_feed) {
        printf("\x1b[A\x1b[A\r");
        sprintf(from_str, "\n\033[1m%s\033[0m [%s]", nickname, datetime_str);
        line_feed = 0;
    } else
        sprintf(from_str, "\r\033[1m%s\033[0m [%s]", nickname, datetime_str);
    printf("%s", from_str);

    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    for (int i = strlen(from_str); i < w.ws_col; i++) {
        putchar(' ');
    }

    printf("\n%s\n\n", message);
    printf("> %s", local_buffer);
    fflush(stdout);
}

void *message_receiver(void *params) {
    int socket_fd = *(int *) params;
    char incoming_nickname[NICKNAME_LENGTH];
    char buffer[BUFFER_LENGTH];
    message_header *msg_header = malloc(sizeof(message_header));

    while (1) {
        if (guard(recvfrom(socket_fd, msg_header, sizeof(message_header), 0, NULL, 0), "[Client] Recv error") == 0) {
            printf("[Receiver] Connection aborted.\n");
            close(socket_fd);
            termination = 1;
            free(msg_header);
            return NULL;
        }

        if (msg_header->type == BC_MSG) {
            // Broadcast
            if (guard(recvfrom(socket_fd, &incoming_nickname, sizeof(incoming_nickname), 0, NULL, 0),
                      "[Client] Recv error") == 0) {
                printf("[Receiver] Connection aborted.\n");
                close(socket_fd);
                termination = 1;
                free(msg_header);
                return NULL;
            }

            if (guard(recvfrom(socket_fd, &buffer, sizeof(char) * msg_header->length, 0, NULL, 0),
                      "[Client] Recv error") == 0) {
                printf("[Receiver] Connection aborted.\n");
                close(socket_fd);
                termination = 1;
                free(msg_header);
                return NULL;
            }
            print_bc_msg(incoming_nickname, buffer);
        } else if (msg_header->type == SV_MSG) {
            // Server message
            if (guard(recvfrom(socket_fd, &buffer, sizeof(char) * msg_header->length, 0, NULL, 0),
                      "[Client] Recv error") == 0) {
                printf("[Receiver] Connection aborted.\n");
                close(socket_fd);
                termination = 1;
                free(msg_header);
                return NULL;
            }
            print_sv_msg(buffer);
        }
        memset(buffer, 0, msg_header->length);
    }
}

int run_client(struct sockaddr_in *server_address, char *nickname) {
    local_buffer = malloc(sizeof(char) * BUFFER_LENGTH); // User input
    buffer_ptr = 0; // Pointer to last buffer char
    int status = 0; // Status received from server
    line_feed = 1; // Did user pressed ENTER
    char c; // New character
    int unicode_sequence = 0; // If unicode sequence is currently fetching
    message_header message_to_send_header; // Message header holder
    message_to_send_header.type = CL_REG;
    message_to_send_header.length = 0;

    server_address->sin_family = AF_INET;
    int socket_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    guard(connect(socket_fd, (struct sockaddr *) server_address, sizeof(struct sockaddr)),
          "[Client] Can't reach server");

    // Registration request: Send header + nickname
    guard(sendto(socket_fd, &message_to_send_header, sizeof(message_header), 0, NULL, 0),
          "[Client] Connection aborted (H)");
    guard(sendto(socket_fd, nickname, NICKNAME_LENGTH, 0, NULL, 0), "[Client] Connection aborted (N)");

    guard(recvfrom(socket_fd, &status, sizeof(int), 0, NULL, 0), "[Client] Registration refused");

    if (status != 1) {
        fprintf(stderr, "[Client] Nickname already exists");
        free(local_buffer);
        exit(1);
    }

    set_terminal_mode();

    pthread_t thread;
    pthread_create(&thread, NULL, message_receiver, &socket_fd);

    getchar(); // Remove trailing \n
    while (1) {
        printf("\n> ");
        fflush(stdout);
        while ((c = getchar()) != '\n') {
            //printf("AAA{%d}\n", c);
            if (termination) { // Received from child thread
                reset_terminal_mode();
                free(local_buffer);
                return 0;
            }
            line_feed = 0;
            if (c == 3) {
                // CTRL+C
                pthread_kill(thread, SIGKILL);
                close(socket_fd);
                reset_terminal_mode();
                free(local_buffer);
                return 0;
            }

            if (c == 27) { // '\'
                // Ignore escape sequences
                if (getchar() == 91) { // '['
                    c = getchar();

                    if (c >= 48 && c <= 57) // '0' - '9'
                        // Assuming that if sequence contains a digit, then there is going to be one more symbol
                        // Example: \[1A
                        getchar();

                    // Otherwise, ignore just one symbol: \[A
                    continue;
                }
            }

            if (c == 127) { // Backspace \b
                if (buffer_ptr > 0) {
                    buffer_ptr--;
                    local_buffer[buffer_ptr] = '\0';
                    printf("\b \b");
                    if (local_buffer[buffer_ptr - 1] == -47 || local_buffer[buffer_ptr - 1] == -48) { // Unicode
                        buffer_ptr--;
                        local_buffer[buffer_ptr] = '\0';
                    } else if (buffer_ptr - 2 >= 0 && local_buffer[buffer_ptr - 2] == -30) { // № symbol
                        local_buffer[buffer_ptr - 1] = '\0';
                        local_buffer[buffer_ptr - 2] = '\0';
                        buffer_ptr -= 2;
                    }
                }
            } else if (((c >= 32 && c <= 126) || c == -47 || c == -48 || c == -30) ||
                       (unicode_sequence && c < 0)) { // ' ' - '~' + UNICODE
                if (buffer_ptr < BUFFER_LENGTH) {
                    local_buffer[buffer_ptr] = c;
                    buffer_ptr++;
                    printf("%c", c);

                    if (c == -47 || c == -48)
                        unicode_sequence = 1; // One more symbol expected
                    else if (c == -30)
                        unicode_sequence = 2; // Two more symbols expected
                    else if (unicode_sequence && c < 0)
                        unicode_sequence--;
                }
            }
        }
        if (termination) { // Received from child thread
            reset_terminal_mode();
            free(local_buffer);
            return 0;
        }
        line_feed = 1;
        if (buffer_ptr == 0) // Empty message
            continue;
        local_buffer[buffer_ptr] = '\0';
        if (strcmp(local_buffer, "!exit") == 0) {
            break;
        }

        message_to_send_header.type = CL_MSG;
        message_to_send_header.length = buffer_ptr;

        guard(sendto(socket_fd, &message_to_send_header, sizeof(message_header), 0, NULL, 0), "[Client] Send error");
        if (guard(sendto(socket_fd, local_buffer, sizeof(char) * message_to_send_header.length, 0, NULL, 0),
                  "[Client] Send error") == 0) {
            printf("[Client] Connection aborted.\n");
            break;
        }
        memset(local_buffer, 0, BUFFER_LENGTH);
        buffer_ptr = 0;
    }

    pthread_kill(thread, SIGKILL);
    close(socket_fd);
    reset_terminal_mode();
    free(local_buffer);
    return 0;
}