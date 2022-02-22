#include "networks.hpp"

int main(int argc, char * argv[]) {
  if (argc != 4) {
    std::cerr << "Usage: ringmaster <port_num> <num_players> <num_hops>" << std::endl;
    return EXIT_FAILURE;
  }

  // Gets the listener socket for current host
  int listener_fd;
  if ((listener_fd = get_listener_socket(argv[1])) == -1) {
    std::cerr << "Error: failed to listen to player connections" << std::endl;
    return EXIT_FAILURE;
  }

  // Gets the number of players and perform error checking
  int num_players = std::stoi(argv[2]);
  if (num_players < 2) {
    std::cerr << "Error: number of players must be greater than 1" << std::endl;
    return EXIT_FAILURE;
  }

  // Gets the number of hops and perform error checking
  int num_hops = std::stoi(argv[3]);
  if (num_hops > 512 || num_hops < 0) {
    std::cerr << "Error: number of hops must be at least 0 and not exceed 512"
              << std::endl;
    return EXIT_FAILURE;
  }
  std::cout << "Potato Ringmaster" << std::endl;
  std::cout << "Players = " << num_players << std::endl;
  std::cout << "Hops = " << num_hops << std::endl;

  // Accepts all player connections
  struct pollfd pfds[num_players];
  char * player_IPs[num_players];
  if (listen_for_player_connections(listener_fd, pfds, num_players, player_IPs) == -1) {
    return EXIT_FAILURE;
  }
  close(listener_fd);

  // Distributes all neighboring player addresses to all players.
  if (distribute_player_addresses(pfds, player_IPs, num_players) == -1) {
    return EXIT_FAILURE;
  }
  delete_player_IPs(player_IPs, num_players);

  if (num_hops == 0) {
    for (int i = 0; i < num_players; i++) {
      close(pfds[i].fd);
    }
    return EXIT_SUCCESS;
  }

  // Sends potato to the first (random) player.
  struct potato potato;
  memset(&potato, 0, sizeof(potato));
  potato.size = 0;
  potato.hops_left = num_hops;
  srand((unsigned int)time(NULL));
  int random_id = std::rand() % num_players;
  std::cout << "Ready to start the game, sending potato to player " << random_id
            << std::endl;
  if (send_buffer(pfds[random_id].fd, &potato, sizeof(potato), 0) == -1) {
    std::cerr << "Error: player " << random_id << " has disconnected" << std::endl;
    return EXIT_FAILURE;
  }

  // Handles end game.
  handle_end_game(pfds, num_players);
  for (int i = 0; i < num_players; i++) {
    close(pfds[i].fd);
  }
  return EXIT_SUCCESS;
}
