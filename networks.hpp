#ifndef __NETWORKS_H__
#define __NETWORKS_H__

#include <arpa/inet.h>
#include <netdb.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cstdlib>
#include <cstring>
#include <exception>
#include <iostream>
#include <vector>

#include "potato.hpp"
#define PORT_BUFFER_SIZE 32
#define BACKLOG 10
int get_port_num(int socket_fd);
void * get_in_addr(struct sockaddr * sa);
ssize_t send_buffer(int target_fd, const void * buf, size_t len, int flags);
void delete_player_IPs(char * player_IPs[], int num_players);
int get_listener_socket(const char * port);
int listen_for_player_connections(int listener_fd,
                                  struct pollfd * pfds,
                                  int num_players,
                                  char ** player_IPs);
int get_connected_socket(const char * hostname, const char * port);
int distribute_player_addresses(struct pollfd * pfds,
                                char ** player_IPs,
                                int num_players);
int accept_neighbor_connection(int listener_fd);
void handle_end_game(struct pollfd * pfds, int num_players);
void play_game(struct pollfd * pfds,
               int pfds_size,
               int player_id,
               int ringmaster_fd,
               int num_players);
#endif
