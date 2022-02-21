#include "networks.hpp"

int main(int argc, char * argv[]) {
  if (argc != 3) {
    std::cerr << "Usage: player <hostname> <port_num>" << std::endl;
    return EXIT_FAILURE;
  }

  // Gets the socket file descriptor connected to the ringmaster.
  int ringmaster_fd;
  if ((ringmaster_fd = get_connected_socket(argv[1], argv[2])) == -1) {
    std::cerr << "Error: failed to connect to ringmaster" << std::endl;
    return EXIT_FAILURE;
  }

  // Receives player id and number of players from ringmaster.
  int player_id, num_players;
  if (recv(ringmaster_fd, &player_id, sizeof(player_id), MSG_WAITALL) <= 0) {
    std::cerr << "Error: disconnected from the ringmaster" << std::endl;
    return EXIT_FAILURE;
  }
  if (recv(ringmaster_fd, &num_players, sizeof(num_players), MSG_WAITALL) <= 0) {
    std::cerr << "Error: disconnected from the ringmaster" << std::endl;
    return EXIT_FAILURE;
  }

  std::cout << "Connected as player " << player_id << " out of " << num_players
            << " total players" << std::endl;

  // Gets the listener socket file descriptor.
  int listener_fd;
  if ((listener_fd = get_listener_socket("")) == -1) {
    std::cerr << "Error: failed to listen to neighboring player's connection"
              << std::endl;
    return EXIT_FAILURE;
  }

  // Gets the port number and send it to ringmaster.
  int port_num;
  if ((port_num = get_port_num(listener_fd)) == -1) {
    return EXIT_FAILURE;
  }
  if (send_buffer(ringmaster_fd, &port_num, sizeof(port_num), 0) == -1) {
    std::cerr << "Error: disconnected from the ringmaster" << std::endl;
    return EXIT_FAILURE;
  }

  // Receives left neighboring player IP address.
  char lft_IP[INET6_ADDRSTRLEN];
  if (recv(ringmaster_fd, lft_IP, INET6_ADDRSTRLEN, MSG_WAITALL) <= 0) {
    std::cerr << "Error: disconnected from the ringmaster" << std::endl;
    return EXIT_FAILURE;
  }

  // Receives left neighboring player port number.
  int lft_port_num;
  if (recv(ringmaster_fd, &lft_port_num, sizeof(lft_port_num), MSG_WAITALL) <= 0) {
    std::cerr << "Error: disconnected from the ringmaster" << std::endl;
    return EXIT_FAILURE;
  }

  // Gets the socket file descriptor connected to the left neighboring player.
  int left_fd;
  if ((left_fd = get_connected_socket(lft_IP, std::to_string(lft_port_num).c_str())) ==
      -1) {
    std::cerr << "Error: failed to connect to neighboring player" << std::endl;
    return EXIT_FAILURE;
  }

  // Gets the socket file descriptor connected to the right neighboring player.
  int right_fd;
  if ((right_fd = accept_neighbor_connection(listener_fd)) == -1) {
    std::cerr << "Error: failed to accept neighboring player's connection" << std::endl;
    return EXIT_FAILURE;
  }
  close(listener_fd);

  struct pollfd pfds[3];  // 3 = 2*neigh + 1*ringmaster
  pfds[0].fd = ringmaster_fd;
  pfds[0].events = POLLIN;
  pfds[1].fd = left_fd;
  pfds[1].events = POLLIN;
  pfds[2].fd = right_fd;
  pfds[2].events = POLLIN;

  // Plays the game.
  play_game(pfds, 3, player_id, ringmaster_fd, num_players);
  close(ringmaster_fd);
  close(left_fd);
  close(right_fd);
  return EXIT_SUCCESS;
}
