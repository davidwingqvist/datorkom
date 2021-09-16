#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
/* You will to add includes here */
#include <netdb.h>
#include <stdint.h>
#include <netinet/in.h>
#include <string.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include <curses.h>
#include <sys/time.h>
#include <string>
#include <time.h>
#include <iostream>
#include <regex.h>
#include <vector>
#include <chrono>

#define MAX_PLAYERS FD_SETSIZE
#define MAX_VIEWER 128
#define MAX_GAMES 9
struct Data
{
  // Check the id of the player.
  int playerId = -1;

  // flags to the server that player wants to join a game.
  int wantToJoin = false;

  int isSpectator = true;
  // The move selection by the player.
  int selection = -1;
};

struct Server_Data
{
  int playerId = -1;
  bool further = false;
};

struct player_data
{
  int playerId = -1;
  bool isSpectator = false;
  bool isSearching = false;
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
  int nrOfPlayers = 0;

  // This is 1 if the main player won, 2 if the opposite side won.
  int winnerId = 0;

  int mainPlayerId = -1;
  int opponentPlayerId = -1;
  int viewerIds[MAX_VIEWER];
};

player_data players[MAX_PLAYERS];
game_player_data playerData[MAX_PLAYERS];
game_data games[MAX_PLAYERS];
int sockets[MAX_PLAYERS];


int produceID()
{
  int ID = rand() % MAX_PLAYERS;
  for(int i = 0; i < MAX_PLAYERS; i++)
  {
    if(ID == players[i].playerId)
    {
      ID = rand() % MAX_PLAYERS;
      i = 0;
    }
  }

  return ID;
}

/*
  RULES
  0 - stone
  1 - sicssor
  2 - bag

  function returns either 1 as in first player or 2 as in second player
*/
int stoneSicssorBag(int first_player, int second_player)
{
  if(first_player == 0 && second_player == 1)
  {
    return 1;
  }
}



// Global values for ease of use.
fd_set current_sockets, ready_sockets, handle_sockets;
int sockfd;
struct sockaddr_in cli;

int main(int argc, char *argv[])
{
	if(argc != 2)
  	{
    	printf("No more/less than 1 argument needs to be present. Retry.\n");
    	exit(0);
  	}

  struct addrinfo hints, *servinfo, *p;
  srand((unsigned int)time(NULL));

  // Divide string into two parts 
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

  
  for(p = servinfo; p != NULL; p = p->ai_next)
  {
      if((sockfd = socket(p->ai_family, p->ai_socktype,
                        p->ai_protocol)) == -1)
      {
        perror("Listener : Socket");
        exit(EXIT_FAILURE);
        continue;
      }

      if(bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
      {
        close(sockfd);
        perror("Listener : Bind");
        exit(EXIT_FAILURE);
        continue;
      }

      break;
  }

  FD_ZERO(&current_sockets);
  FD_SET(sockfd, &current_sockets);

  freeaddrinfo(servinfo);
  //int flags = fcntl(connfd, F_GETFL, 0);
	//fcntl(connfd, F_SETFL, flags | O_NONBLOCK);

  socklen_t len = sizeof cli;
  struct itimerval alarmTime;
  alarmTime.it_interval.tv_sec=10;
  alarmTime.it_interval.tv_usec=10;
  alarmTime.it_value.tv_sec=10;
  alarmTime.it_value.tv_usec=10;

  listen(sockfd, MAX_PLAYERS);
  
  while(1)
  {
  ready_sockets = current_sockets;
  handle_sockets = current_sockets;
  int sel = select(FD_SETSIZE, &ready_sockets, &handle_sockets, NULL, NULL);

  switch (sel)
  {
    case -1: 
    std::cout << "Error with connection.\n";
    exit(EXIT_FAILURE);

    break;
    case 0:
    // Timeout.
    //std::cout << "Connection timed out.\n";

    break;
    default:

    
    for(int i = 0; i < FD_SETSIZE; i++)
    {
      // write sockets.
      if(FD_ISSET(i, &ready_sockets))
      {
        // A new connection.
        if(i == sockfd)
        {
          socklen_t len = sizeof(cli);
          int client_socket = accept(sockfd, (struct sockaddr*)&cli, &len);
          FD_SET(client_socket, &current_sockets);
          sockets[i] = client_socket;
          players[i].playerId = produceID();
          std::cout << "New connection found.\nID given: " << players[i].playerId << "\n";
          Server_Data data;
          data.playerId = players[i].playerId;
          int wr = write(client_socket, &data, sizeof(data));
          if(wr == -1)
          {
            std::cout << "Error when reporting ID to player " << i << ".\n";
          }
        }
        else
        {
          Data data;
          int rd = read(i, &data, sizeof(data));
          if(rd == 0)
          {
            printf("User has disconnected.\n");
            players[i].playerId = -1;
            FD_CLR(i, &current_sockets);
            close(i);
            continue;
          }
          else
          {
            std::cout << "Client: " << data.playerId << " wants to join? - " << data.wantToJoin << "\n";
          }
        }
      }
    }
  }
  }

  close(sockfd);
  return EXIT_SUCCESS;
}