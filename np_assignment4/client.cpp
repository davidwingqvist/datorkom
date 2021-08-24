#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
/* You will to add includes here */
#include <string.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <curses.h>
#include <unistd.h>
#include <vector>
#include <iostream>
#include <string>
#include <regex.h>

#define STDIN 0
#define MAX_VIEWER 5

void MainMenu();


struct state
{
  // This is 0 if in menu, this is 1 if in game, this is 2 if is spectating a game.
  int current_state = 0;
}client_state;

struct player_data
{
  int playerId = -1;
  bool isSpectator = false;
};

struct game_player_data
{
  player_data player;
  int score = 0;
  // 1 - scissor, 2 - Stone, 3 - Paper, 0 == No choice was made.
  int current_choice = 0;
};

struct game_data
{
  // Check if the game is filled with 2 players yet.
  bool isFull = false;

  // This is 1 if the main player won, 2 if the opposite side won.
  int winnerId = 0;

  int mainPlayerId = -1;
  int opponentPlayerId = -1;
  int viewerIds[MAX_VIEWER];
};

fd_set current_sockets, ready_sockets;

int main(int argc, char *argv[]){
  
  	/* Do magic */
	if(argc != 2)
  {
    printf("ONLY 2 ARGUMENTS ARE ACCEPTED\n PROGRAM IP:PORT\n");
    exit(0);
  }

  struct addrinfo hints, *servinfo, *p;
  char* address = strtok(argv[1], ":");
  char* port = strtok(NULL, "");

  printf("Host %s ", address);
  printf("and port %s\n", port);
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_CANONNAME;

  int rv;
  if((rv = getaddrinfo(address, port, &hints, &servinfo)) != 0)
  {
      perror("Address info");
      exit(0);
  }

  int sockfd;
  for(p = servinfo; p != NULL; p = p->ai_next)
  {
      if((sockfd = socket(p->ai_family, p->ai_socktype,
                        p->ai_protocol)) == -1)
      {
        perror("Listener : Socket");
        close(sockfd);
        exit(EXIT_FAILURE);
        continue;
      }

      if(connect(sockfd, p->ai_addr, p->ai_addrlen) == -1)
      {
        close(sockfd);
        perror("Listener : Connect");
        exit(EXIT_FAILURE);
        continue;
      }

      break;
  }

  FD_ZERO(&current_sockets);
  FD_SET(STDIN, &current_sockets);
  FD_SET(sockfd, &current_sockets);

  freeaddrinfo(servinfo);

  MainMenu();

  while(1)
  {
    ready_sockets = current_sockets;
    int sel = select(FD_SETSIZE, &ready_sockets, NULL, NULL, NULL);
    switch(sel)
    {
      case 0:
      // Timeout.
      exit(EXIT_FAILURE);
      break;
      case -1:
      std::cout << "Timeout from server.\n";
      exit(EXIT_FAILURE);
      break;
      default:

      for(int i = 0; i < FD_SETSIZE; i++)
      {
        if(FD_ISSET(i, &ready_sockets))
        {
          // input
          if(i == 0)
          {
            char input[2] = {-1};
            fgets(input, 2, stdin);
            switch(client_state.current_state)
            {
              case 0:
              std::cout << input << "\n";
              if(strcmp(input, "2") == NULL)
              {
                close(sockfd);
                exit(EXIT_SUCCESS);
              }
              break;
              case 1:
              break;
              case 2:
              break;
            }

          }
          // Read for messages.
          else if(i == sockfd)
          {
            int r = 1;//read(sockfd, &message, sizeof(message));
            if(r > 0)
            {

            }
            else
            {
              // Disconnect.
              std::cout << "Server shutdown.\n";
              close(sockfd);
              exit(EXIT_SUCCESS);
            }
          }
        }
      }

      break;
    }
	  
  }

  close(sockfd);
  return EXIT_SUCCESS;

}

void MainMenu()
{
  std::cout << "Welcome to the Rock, Paper, Scissor Game!\n";
  std::cout << "Please choose one of the options below.\n";

  std::cout << "[0] - Join a Game.\n[1] - Spectate a Game.\n[2] - Quit Game\n";
}