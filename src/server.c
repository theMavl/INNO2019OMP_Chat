#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <memory.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <dirent.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <pthread.h>
#include <sys/poll.h>
#include <sys/ioctl.h>


#include "server.h"

struct user_hashed *connected_users;

int active_users;

void delete_users() {
    struct user_hashed *current_user, *tmp;
    HASH_ITER(hh, connected_users, current_user, tmp) {
        HASH_DEL(connected_users, current_user);
        close(current_user->sock_fd);
        free(current_user);
    }
}

int find_free_poll_slot(struct pollfd *fds) {
    for (int i = 0; i < MAX_USERS; i++) {
        if (fds[i].fd < 0)
            return i;
    }
    return -1;
}

int nick_exists(char *nick) {
    for (struct user_hashed *tmp_user = connected_users; tmp_user != NULL; tmp_user = tmp_user->hh.next) {
        if (strcmp(nick, tmp_user->nickname) == 0)
            return 1;
    }
    return 0;
}

/**
 * Send given message to all connected users
 * @param header message header (size and payload length)
 * @param from typically a nickname (ignored in case of SV_MSG)
 * @param message payload
 */
void broadcast_msg(message_header *header, const char *from, const char *message) {
    struct user_hashed *tmp_user;
    for (tmp_user = connected_users; tmp_user != NULL; tmp_user = tmp_user->hh.next) {
        guard(sendto(tmp_user->sock_fd, header, sizeof(header), 0,
                     NULL, 0), "[Server] Broadcast error");

        if (from != NULL && header->type == BC_MSG) {
            guard(sendto(tmp_user->sock_fd, from, NICKNAME_LENGTH, 0, NULL, 0),
                  "[Server] Broadcast error");
        }

        guard(sendto(tmp_user->sock_fd, message, sizeof(char) * header->length,
                     0, NULL, 0), "[Server] Broadcast error");
        // printf("Sending '%s' to %s (%d)\n", message, addr_to_str(tmp_user->address), tmp_user->sock_fd);
    }
}

void run_server(struct sockaddr_in *server_addr) {
    active_users = 0;

    struct pollfd fds[MAX_USERS]; // Sockets for existing users
    int master_socket_fd = 0; // Socket for new users

    // Temps
    socklen_t addr_len = sizeof(struct sockaddr);
    int tmp_fd, bytes_transferred;
    char tmp_nickname[NICKNAME_LENGTH];
    struct sockaddr_in tmp_user_address;
    struct user_hashed *tmp_user;
    message_header *message_tmp_header = malloc(sizeof(message_header));

    char buffer[BUFFER_LENGTH];
    memset(buffer, 0, BUFFER_LENGTH);

    connected_users = NULL; // Initialize users hashtable

    // Prepare server
    master_socket_fd = guard(socket(AF_INET, SOCK_STREAM, IPPROTO_TCP), "[Server] Socket creation failed");
    guard(setsockopt(master_socket_fd, SOL_SOCKET, SO_REUSEADDR, &(int) {1}, sizeof(int)),
          "[Server] setsockopt(SO_REUSEADDR) failed");
    guard(bind(master_socket_fd, (struct sockaddr *) server_addr, addr_len), "[Server] Socket bind failed");
    guard(listen(master_socket_fd, MAX_USERS - active_users), "[Server] Listen failed");

    // Set unused slots as -1
    for (int i = 1; i < MAX_USERS; i++)
        fds[i].fd = -1;

    // Add master socket to poll
    fds[0].fd = master_socket_fd;
    fds[0].events = POLLIN;

    while (1) {
        printf("[Server] Poll...\n");
        guard(poll(fds, MAX_USERS, -1), "[Server] Poll error");

        for (int i = 0; i < MAX_USERS; i++) {
            if (fds[i].fd == -1 || fds[i].revents != POLLIN)
                continue;
            // Socket must be active, event POLLIN
            if (fds[i].fd == master_socket_fd) {
                // New user
                printf("[Server] Connection incoming\n");
                tmp_fd = find_free_poll_slot(fds);
                fds[tmp_fd].fd = guard(accept(master_socket_fd, (struct sockaddr *) &tmp_user_address, &addr_len),
                                       "[Server] Accept error");
                printf("[Server] Accepted %s\n", addr_to_str(tmp_user_address));
                fds[tmp_fd].events = POLLIN;
                active_users++;
                tmp_user = malloc(sizeof(struct user_hashed));
                tmp_user->address = tmp_user_address;
                tmp_user->sock_fd = fds[tmp_fd].fd;
                tmp_user->registered = 0;
                memset(tmp_user->nickname, 0, NICKNAME_LENGTH);
                HASH_ADD_INT(connected_users, sock_fd, tmp_user);
            } else {
                // Existing user
                bytes_transferred = guard(recv(fds[i].fd, message_tmp_header, sizeof(message_header), 0),
                                          "[Server] Receive failed");
                HASH_FIND_INT(connected_users, &fds[i].fd, tmp_user);

                if (tmp_user == NULL) continue;

                printf("[Server] Incoming %d bytes from %s. ", bytes_transferred, addr_to_str(tmp_user->address));

                if (bytes_transferred == 0) {
                    // Close the connection
                    int user_registered = tmp_user->registered;
                    if (user_registered) {
                        sprintf(buffer, "%s left the chat", tmp_user->nickname);
                        message_tmp_header->type = SV_MSG;
                        message_tmp_header->length = strlen(buffer);
                    }

                    printf("\n[Server] Closed the connection with peer %s[%s]\n", tmp_user->nickname,
                           addr_to_str(tmp_user->address));
                    HASH_DEL(connected_users, tmp_user);
                    close(tmp_user->sock_fd);
                    free(tmp_user);
                    fds[i].fd = -1;

                    if (user_registered) {
                        // Broadcast user's leave
                        broadcast_msg(message_tmp_header, NULL, buffer);
                        memset(buffer, 0, BUFFER_LENGTH);
                    }
                    continue;
                }

                printf("Message type: %d, length: %d\n", message_tmp_header->type, message_tmp_header->length);

                if (message_tmp_header->type == CL_REG) {
                    // User finishes registration (sends his nickname)
                    memset(tmp_nickname, 0, NICKNAME_LENGTH);
                    bytes_transferred = guard(recv(fds[i].fd, tmp_nickname, NICKNAME_LENGTH, 0),
                                              "[Server] Receive failed");
                    if (bytes_transferred != 0) {
                        if (nick_exists(tmp_nickname)) {
                            sendto(tmp_user->sock_fd, &(int) {2}, sizeof(int), 0, NULL, 0);
                            printf("[Server] %s tried to change nickname to %s, but it exists\n",
                                   addr_to_str(tmp_user->address), tmp_user->nickname);
                        } else {
                            printf("[Server] %s changed nickname to %s\n", addr_to_str(tmp_user->address),
                                   tmp_user->nickname);
                            sendto(tmp_user->sock_fd, &(int) {1}, sizeof(int), 0, NULL, 0);

                            if (tmp_user->registered) {
                                sprintf(buffer, "%s changed nickname to %s", tmp_user->nickname, tmp_nickname);
                            } else {
                                sprintf(buffer, "%s joined the chat", tmp_nickname);
                            }

                            strcpy(tmp_user->nickname, tmp_nickname);
                            tmp_user->registered = 1;

                            message_tmp_header->type = SV_MSG;
                            message_tmp_header->length = strlen(buffer);
                            broadcast_msg(message_tmp_header, NULL, buffer);
                        }
                    }
                } else if (message_tmp_header->type == CL_MSG) {
                    // User sends a message
                    if (!tmp_user->registered) {
                        // Message from unregistered user (nickname is unknown) with CL_MSG type
                        message_tmp_header->type = SV_MSG;
                        message_tmp_header->length = -1;
                        sendto(tmp_user->sock_fd, message_tmp_header, sizeof(message_header), 0, NULL, 0);
                        continue;
                    }
                    bytes_transferred = guard(recv(fds[i].fd, &buffer, BUFFER_LENGTH, 0), "[Server] Receive failed");
                    printf("[Server] Incoming message (%d bytes) from %s\n", bytes_transferred,
                           addr_to_str(tmp_user->address));

                    message_tmp_header->length = strlen(buffer);
                    message_tmp_header->type = BC_MSG;

                    printf("[Server] Length: %d, Message: %s\n", message_tmp_header->length, buffer);

                    strcpy(tmp_nickname, tmp_user->nickname);

                    broadcast_msg(message_tmp_header, tmp_nickname, buffer);
                }
                memset(buffer, 0, BUFFER_LENGTH);
            }
        }
    }

    close(master_socket_fd);
    delete_users();
    free(message_tmp_header);
}