#include "networks.hpp"

int main(int argc, char * argv[]) {
  if (argc != 4) {
    std::cerr << "Usage: ringmaster <port_num> <num_players> <num_hops>" << std::endl;
    return EXIT_FAILURE;
  }

  int listener_fd;
  if ((listener_fd = get_listener_socket(argv[1])) == -1) {
    std::cerr << "Error: failed to listen to player connections" << std::endl;
    return EXIT_FAILURE;
  }
  int num_players = std::stoi(argv[2]);
  if (num_players < 2) {
    std::cerr << "Error: number of players must be at least 2" << std::endl;
    return EXIT_FAILURE;
  }
  int num_hops = std::stoi(argv[3]);
  if (num_hops > 512) {
    std::cerr << "Error: number of hops must not exceed 512" << std::endl;
    return EXIT_FAILURE;
  }
  std::cout << "Potato Ringmaster" << std::endl;
  std::cout << "Players = " << num_players << std::endl;
  std::cout << "Hops = " << num_hops << std::endl;

  struct pollfd pfds[num_players];
  char * player_IPs[num_players];
  if (listen_for_player_connections(listener_fd, pfds, num_players, player_IPs) == -1) {
    return EXIT_FAILURE;
  }
  close(listener_fd);
  if (distribute_player_addresses(pfds, player_IPs, num_players) == -1) {
    return EXIT_FAILURE;
  }
  delete_player_IPs(player_IPs, num_players);

  struct potato potato;
  memset(&potato, 0, sizeof(potato));
  potato.size = 0;
  potato.hops_left = num_hops;
  int random_id = std::rand() % num_players;
  if (send_buffer(pfds[random_id].fd, &potato, sizeof(potato), 0) == -1) {
    std::cerr << "Error: player " << random_id << " has disconnected" << std::endl;
    return EXIT_FAILURE;
  }
  std::cout << "Ready to start the game, sending potato to player " << random_id
            << std::endl;

  handle_end_game(pfds, num_players);
  for (int i = 0; i < num_players; i++) {
    close(pfds[i].fd);
  }
  return EXIT_SUCCESS;
}
