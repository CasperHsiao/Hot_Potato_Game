#include "networks.hpp"

/*
 * Gets the port number of the given socket file descriptor.
 * Returns -1 on error.
 */
int get_port_num(int socket_fd) {
  struct sockaddr_in sin;
  socklen_t socklen = sizeof(sin);
  if (getsockname(socket_fd, (struct sockaddr *)&sin, &socklen) == -1) {
    std::cerr << "Error: failed to get port number" << std::endl;
    return -1;
  }
  return ntohs(sin.sin_port);
}

/*
 * Returns socket internet address.
 */
void * get_in_addr(struct sockaddr * sa) {
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in *)sa)->sin_addr);
  }

  return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

/*
 * A send() wrapper function that ensures the specified amount of bytes are sent.
 * Returns -1 on error.
 */
ssize_t send_buffer(int target_fd, const void * buf, size_t len, int flags) {
  ssize_t bytes_sent;
  ssize_t total_bytes_sent = 0;
  size_t bytes_left = len;
  char * buf_left = (char *)buf;
  while (bytes_left > 0) {
    if ((bytes_sent = send(target_fd, buf_left, bytes_left, flags)) == -1) {
      std::cerr << "Error: failed to send entire buffer" << std::endl;
      return -1;
    }
    total_bytes_sent += bytes_sent;
    bytes_left -= bytes_sent;
    buf_left += bytes_sent;
  }
  return total_bytes_sent;
}

/*
 * Deletes the given array of player IPs.
 */
void delete_player_IPs(char * player_IPs[], int num_players) {
  for (int i = 0; i < num_players; i++) {
    delete[] player_IPs[i];
  }
}

/*
 * Gets the file descriptor of the listener socket on localhost.
 * Returns -1 on error.
 */
int get_listener_socket(const char * port) {
  int sock_fd;
  struct addrinfo host_info, *host_info_list, *p;
  const char * hostname = NULL;
  int yes = 1;
  int rv;

  memset(&host_info, 0, sizeof(host_info));
  host_info.ai_family = AF_UNSPEC;
  host_info.ai_socktype = SOCK_STREAM;
  host_info.ai_flags = AI_PASSIVE;  // use my IP

  if ((rv = getaddrinfo(hostname, port, &host_info, &host_info_list)) != 0) {
    std::cerr << "Error: failed to get address info from the host" << std::endl;
    std::cerr << "  (" << hostname << "," << port << ")" << std::endl;
    return -1;
  }

  // loop through all the results and bind to the first we can
  for (p = host_info_list; p != NULL; p = p->ai_next) {
    if (strcmp(port, "") == 0) {
      struct sockaddr_in * addr_in = (struct sockaddr_in *)(host_info_list->ai_addr);
      addr_in->sin_port = 0;
    }
    if ((sock_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
      std::cerr << "Error: failed to create server socket" << std::endl;
      std::cerr << "  (" << hostname << "," << port << ")" << std::endl;
      continue;
    }

    if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
      std::cerr << "Error: failed to set socket option to allow address reuse"
                << std::endl;
      std::cerr << "  (" << hostname << "," << port << ")" << std::endl;
      return -1;
    }

    if (bind(sock_fd, p->ai_addr, p->ai_addrlen) == -1) {
      close(sock_fd);
      std::cerr << "Error: failed to bind socket to the host" << std::endl;
      std::cerr << "  (" << hostname << "," << port << ")" << std::endl;
      continue;
    }
    break;
  }

  if (p == NULL) {
    std::cerr << "Error: failed to create a listener socket" << std::endl;
    return -1;
  }

  if (listen(sock_fd, BACKLOG) == -1) {
    std::cerr << "Error: failed to listen on the server socket" << std::endl;
    return -1;
  }
  freeaddrinfo(host_info_list);  // all done with this structure

  return sock_fd;
}

/*
 * Listens for player connection with the given amount of players.
 * Stores the connected socket file descriptor into pfds and client IP into player_IPs.
 * Returns -1 on error.
 */
int listen_for_player_connections(int listener_fd,
                                  struct pollfd * pfds,
                                  int num_players,
                                  char ** player_IPs) {  // ONLY accept once for now
  int pfds_size = 0;
  int client_fd;
  struct sockaddr_storage client_addr;  // connector's address information
  socklen_t addrlen = sizeof(client_addr);
  struct pollfd * listener_pfd = new struct pollfd();
  listener_pfd[0].fd = listener_fd;
  listener_pfd[0].events = POLLIN;
  while (pfds_size < num_players) {
    int poll_count = poll(listener_pfd, 1, -1);
    if (poll_count == -1) {
      std::cerr << "Error: poll_count accept error" << std::endl;
      throw std::exception();
    }

    client_fd = accept(listener_pfd[0].fd, (struct sockaddr *)&client_addr, &addrlen);
    char * clientIP = new char[INET6_ADDRSTRLEN];
    memset(clientIP, 0, INET6_ADDRSTRLEN);
    inet_ntop(client_addr.ss_family,
              get_in_addr((struct sockaddr *)&client_addr),
              clientIP,
              INET6_ADDRSTRLEN);
    if (strcmp(clientIP, "127.0.0.1") == 0) {
      if (gethostname(clientIP, INET6_ADDRSTRLEN) == -1) {
        std::cerr << "Error: failed to get client IP" << std::endl;
        return -1;
      }
    }

    if (client_fd == -1) {
      std::cerr << "Error: failed to accpet client connection from " << clientIP
                << std::endl;
    }
    else {
      std::cout << "Player " << pfds_size << " is ready to play" << std::endl;
      if (send_buffer(client_fd, &pfds_size, sizeof(pfds_size), 0) == -1) {
        std::cerr << "Player " << pfds_size << " has disconnected" << std::endl;
        return -1;
      }
      if (send_buffer(client_fd, &num_players, sizeof(num_players), 0) == -1) {
        std::cerr << "Player " << pfds_size << " has disconnected" << std::endl;
        return -1;
      }
      pfds[pfds_size].fd = client_fd;
      pfds[pfds_size].events = POLLIN;
      player_IPs[pfds_size] = clientIP;
      pfds_size++;
    }
  }
  delete listener_pfd;
  return 0;
}

/*
 * Gets the socket file descriptor connected to the given hostname and port.
 * Returns -1 on error.
 */
int get_connected_socket(const char * hostname, const char * port) {
  int server_fd;
  struct addrinfo server_info, *server_info_list, *p;
  char serverIP[INET6_ADDRSTRLEN];
  int rv;

  memset(&server_info, 0, sizeof(server_info));
  server_info.ai_family = AF_UNSPEC;
  server_info.ai_socktype = SOCK_STREAM;
  if ((rv = getaddrinfo(hostname, port, &server_info, &server_info_list)) != 0) {
    std::cerr << "Error: cannot get address info for server" << std::endl;
    std::cerr << "  (" << hostname << "," << port << ")" << std::endl;
    return -1;
  }

  // loop through all the results and connect to the first we can
  for (p = server_info_list; p != NULL; p = p->ai_next) {
    if ((server_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
      std::cerr << "Error: cannot create socket" << std::endl;
      std::cerr << "  (" << hostname << "," << port << ")" << std::endl;
      continue;
    }

    if (connect(server_fd, p->ai_addr, p->ai_addrlen) == -1) {
      close(server_fd);
      std::cerr << "Error: cannot connect to server" << std::endl;
      std::cerr << "  (" << hostname << "," << port << ")" << std::endl;
      continue;
    }
    break;
  }

  if (p == NULL) {
    return -1;
  }

  inet_ntop(p->ai_family,
            get_in_addr((struct sockaddr *)p->ai_addr),
            serverIP,
            sizeof(serverIP));
  freeaddrinfo(server_info_list);  // all done with this structure
  return server_fd;
}

/*
 * Distributes player's neighboring player addresses.
 * Returns -1 on error.
 */
int distribute_player_addresses(struct pollfd * pfds,
                                char ** player_IPs,
                                int num_players) {
  std::vector<int> hasRecv(num_players, 0);
  int count = 0;
  int port_num;
  while (count < num_players) {
    int poll_count = poll(pfds, num_players, -1);
    if (poll_count == -1) {
      std::cerr << "Error: poll_count distribute port numbers error" << std::endl;
      throw std::exception();
    }

    for (int i = 0; i < num_players; i++) {
      if (pfds[i].revents & POLLIN && hasRecv[i] == 0) {
        if (recv(pfds[i].fd, &port_num, sizeof(port_num), MSG_WAITALL) <= 0) {
          std::cerr << "Error: player " << i << " has disconnected" << std::endl;
          return -1;
        }
        char * IP = player_IPs[i];
        int rgt_neigh_index = (i + 1) % num_players;
        if (send_buffer(pfds[rgt_neigh_index].fd, IP, INET6_ADDRSTRLEN, 0) == -1) {
          std::cerr << "Error: player " << rgt_neigh_index << " has disconnected"
                    << std::endl;
          return -1;
        }
        if (send_buffer(pfds[rgt_neigh_index].fd, &port_num, sizeof(port_num), 0) == -1) {
          std::cerr << "Error: player " << rgt_neigh_index << " has disconnected"
                    << std::endl;
          return -1;
        }
        count++;
        hasRecv[i] = 1;
      }
    }
  }
  return 0;
}

/*
 * Gets the file descriptor of the accepted connection.
 * Returns -1 on error.
 */
int accept_neighbor_connection(int listener_fd) {
  struct sockaddr_storage client_addr;  // connector's address information
  socklen_t addrlen = sizeof(client_addr);
  return accept(listener_fd, (struct sockaddr *)&client_addr, &addrlen);
}

/*
 * Ringmaster prints the trace of potato and announces end game to all players.
 */
void handle_end_game(struct pollfd * pfds, int num_players) {
  struct potato potato;
  memset(&potato, 0, sizeof(potato));
  while (true) {
    int poll_count = poll(pfds, num_players, -1);
    if (poll_count == -1) {
      std::cerr << "Error: poll_count handle end game error" << std::endl;
      throw std::exception();
    }

    for (int i = 0; i < num_players; i++) {
      if (pfds[i].revents & POLLIN) {
        if (recv(pfds[i].fd, &potato, sizeof(potato), MSG_WAITALL) > 0) {
          std::cout << "Trace of potato:" << std::endl;
          std::string sep = "";
          for (int i = 0; i < potato.size; i++) {
            std::cout << sep << potato.trace[i];
            sep = ",";
          }
          std::cout << std::endl;
          for (int j = 0; j < num_players; j++) {
            send_buffer(pfds[j].fd, &potato, sizeof(potato), 0);
          }
          return;
        }
        else {
          std::cerr << "Error: player " << i << " has disconnected" << std::endl;
          return;
        }
      }
    }
  }
}

/*
 * Player plays the game.
 */
void play_game(struct pollfd * pfds,
               int pfds_size,
               int player_id,
               int ringmaster_fd,
               int num_players) {
  struct potato potato;
  std::srand((unsigned int)time(NULL) + player_id);
  while (true) {
    int poll_count = poll(pfds, pfds_size, -1);
    if (poll_count == -1) {
      std::cerr << "Error: poll_count play game error" << std::endl;
      throw std::exception();
    }
    for (int i = 0; i < pfds_size; i++) {
      if (pfds[i].revents & POLLIN) {
        if (recv(pfds[i].fd, &potato, sizeof(potato), MSG_WAITALL) > 0) {
          if (potato.hops_left == -1) {
            return;
          }
          else if (potato.hops_left == 0) {
            potato.trace[potato.size] = player_id;
            potato.size++;
            potato.hops_left--;
            std::cout << "I'm it" << std::endl;
            send_buffer(ringmaster_fd, &potato, sizeof(potato), 0);
          }
          else {
            potato.trace[potato.size] = player_id;
            potato.size++;
            potato.hops_left--;
            int random_index = std::rand() % 2;  // 0 or 1
            int neigh_id = (player_id + 1) % num_players;
            if (random_index == 0) {
              neigh_id = player_id - 1;
              if (neigh_id == -1) {
                neigh_id = num_players - 1;
              }
            }
            else {
              neigh_id = (player_id + 1) % num_players;
            }
            std::cout << "Sending potato to " << neigh_id << std::endl;
            send_buffer(pfds[2].fd, &potato, sizeof(potato), 0);
          }
        }
        else if (pfds[i].fd == ringmaster_fd) {
          std::cerr << "Error: disconnected from ringaster" << std::endl;
          return;
        }
      }
    }
  }
}
