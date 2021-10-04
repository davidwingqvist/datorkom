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

struct compressed_game
{
  int main = 0;
  int opp = 0;
  int id = -1;
};

struct game_data
{
  // Check if the game is filled with 2 players yet.
  int nrOfPlayers = 0;

  int mainPlayerChoice = -1;
  int opponentPlayerChoice = -1;

  int mainPlayerScore = 0;
  int opponentPlayerScore = 0;

  // This is 1 if the main player won, 2 if the opposite side won.
  int winnerId = 0;

  int mainPlayerId = -1;
  int opponentPlayerId = -1;
  int viewerIds[MAX_VIEWER] = {0};

  std::chrono::steady_clock::time_point startTime;
  std::chrono::steady_clock::time_point choiceStartTime;
  bool resetTime = true;
  int timer = 1;
  float mainPlayerRespTime = 0.0f;
  float opponentRespTime = 0.0f;
  int totalRounds = 0;
  bool isChoiceRound = false;
};

struct Data
{
  // Check the id of the player.
  int playerId = -1;

  // flags to the server that player wants to join a game.
  int wantToJoin = false;

  int wantToSpectate = false;

  int isSpectator = true;
  // The move selection by the player.
  int selection = -1;
};

struct Server_Data
{
  int playerId = -1;
  int whatToRead = -1;

  bool gameFound = false;

  int timeLeft = 0;

  bool newRound = false;

  int score = 0;
  
  compressed_game games[MAX_GAMES];
};

struct player_data
{
  int playerId = -1;
  bool isSpectator = false;
  int isSearching = false;
  int playerSocket = -1;
  int gameID = -1;
};

struct game_player_data
{
  player_data player;
  int score = 0;
  // 1 - scissor, 2 - Stone, 3 - Paper, 0 == No choice was made.
  int current_choice = 0;
};

player_data players[FD_SETSIZE];
game_player_data playerData[FD_SETSIZE];
game_data games[FD_SETSIZE];

int produceID()
{
  int ID = rand() % MAX_PLAYERS;
  for (int i = 0; i < MAX_PLAYERS; i++)
  {
    if (ID == players[i].playerId)
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
  if (first_player == -1 && second_player == -1)
  {
    return 0;
  }
  if (first_player == -1)
    return 2;
  else if (second_player == -1)
    return 1;

  if (first_player == 0 && second_player == 1)
  {
    return 1;
  }
  else if (first_player == 0 && second_player == 2)
  {
    return 2;
  }
  else if (first_player == 1 && second_player == 0)
  {
    return 2;
  }
  else if (first_player == 1 && second_player == 2)
  {
    return 1;
  }
  else if (first_player == 2 && second_player == 0)
  {
    return 1;
  }
  else if (first_player == 2 && second_player == 1)
  {
    return 2;
  }

  else if (second_player == 0 && first_player == 1)
  {
    return 2;
  }
  else if (second_player == 0 && first_player == 2)
  {
    return 1;
  }
  else if (second_player == 1 && first_player == 0)
  {
    return 1;
  }
  else if (second_player == 1 && first_player == 2)
  {
    return 2;
  }
  else if (second_player == 2 && first_player == 0)
  {
    return 2;
  }
  else if (second_player == 2 && first_player == 1)
  {
    return 1;
  }

  return -1;
}

/*
  Returns ID of player found that is also searching.
*/
int searchForOpponent(int searche)
{
  for (int i = 0; i < FD_SETSIZE; i++)
  {
    if (players[i].isSearching && i != searche && players[i].isSpectator == false)
    {
      return i;
    }
  }

  return -1;
}

auto clock_time = std::chrono::steady_clock::now();
// Global values for ease of use.
fd_set current_sockets, ready_sockets, handle_sockets;
int sockfd;
struct sockaddr_in cli;

int main(int argc, char *argv[])
{
  if (argc != 2)
  {
    printf("No more/less than 1 argument needs to be present. Retry.\n");
    exit(0);
  }

  struct addrinfo hints, *servinfo, *p;
  srand((unsigned int)time(NULL));

  // Divide string into two parts
  char *address = strtok(argv[1], ":");
  char *port = strtok(NULL, "");
  printf("Host %s ", address);
  printf("and port %s\n", port);

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_CANONNAME;

  int rv;
  if ((rv = getaddrinfo(address, port, &hints, &servinfo)) != 0)
  {
    perror("Address info");
    exit(0);
  }

  for (p = servinfo; p != NULL; p = p->ai_next)
  {
    if ((sockfd = socket(p->ai_family, p->ai_socktype,
                         p->ai_protocol)) == -1)
    {
      perror("Listener : Socket");
      exit(EXIT_FAILURE);
      continue;
    }

    if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
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

  socklen_t len = sizeof cli;
  struct itimerval alarmTime;
  alarmTime.it_interval.tv_sec = 10;
  alarmTime.it_interval.tv_usec = 10;
  alarmTime.it_value.tv_sec = 10;
  alarmTime.it_value.tv_usec = 10;

  listen(sockfd, MAX_PLAYERS);

  while (1)
  {
    clock_time = std::chrono::steady_clock::now();
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

      for (int i = 0; i < FD_SETSIZE; i++)
      {
        if (players[i].isSearching == 1)
        {
          int o = searchForOpponent(i);

          // Opponent found.
          if (o != -1)
          {
            std::cout << "Game has been found for " << i << " and " << o << " !\n";
            players[i].isSearching = false;
            players[o].isSearching = false;
            games[i].mainPlayerId = i;
            games[i].opponentPlayerId = o;

            players[i].gameID = i;
            players[o].gameID = i;
            int gameID = i;

            std::cout << "GAME ID: " << i << "\n";

            // SETUP game
            games[gameID].nrOfPlayers = 2;
            games[gameID].mainPlayerId = i;
            games[gameID].opponentPlayerId = o;

            // Announce to players that match has been found.

            // First player
            Server_Data data;
            data.playerId = players[i].playerId;
            data.whatToRead = 0;
            data.gameFound = true;

            int wr = write(i, &data, sizeof(data));
            if (wr == -1)
            {
              perror("Write: ");
              std::cout << "Game invitation 1 failed\n";
            }

            // Second player
            data.playerId = players[o].playerId;
            data.whatToRead = 0;
            data.gameFound = true;

            wr = write(o, &data, sizeof(data));
            if (wr == -1)
            {
              std::cout << "Game invitation 2 failed\n";
            }
          }
        }

        // Update games.
        if (games[i].nrOfPlayers == 2)
        {

          // On player win game.
          if (games[i].mainPlayerScore >= 3 || games[i].opponentPlayerScore >= 3)
          {
            int main = games[i].mainPlayerScore;
            int opp = games[i].opponentPlayerScore;
            if (main >= 3)
            {
              // Main player win.
              Server_Data winGame;
              winGame.whatToRead = 8;

              write(games[i].mainPlayerId, &winGame, sizeof(Server_Data));

              winGame.whatToRead = 7;
              write(games[i].opponentPlayerId, &winGame, sizeof(Server_Data));
            }
            else if (opp >= 3)
            {
              // Opponent win.
              Server_Data winGame;
              winGame.whatToRead = 7;

              write(games[i].mainPlayerId, &winGame, sizeof(Server_Data));

              winGame.whatToRead = 8;
              write(games[i].opponentPlayerId, &winGame, sizeof(Server_Data));
            }

            // Print out reaction times on server.
            std::cout << "------------------------------------\n";
            std::cout << "Reaction times for Game with ID: " << i << "\n";
            std::cout << "Main player " << games[i].mainPlayerId << ": " << abs(games[i].mainPlayerRespTime / games[i].totalRounds) << ".\n";
            std::cout << "Opponent player " << games[i].opponentPlayerId << ": " << abs(games[i].opponentRespTime / games[i].totalRounds) << ".\n";
            std::cout << "------------------------------------\n";
            // Reset game.
            game_data newGame;
            games[i] = newGame;
          }

          // On new Round
          if (games[i].resetTime)
          {
            games[i].startTime = std::chrono::steady_clock::now();
            games[i].resetTime = false;

            Server_Data roundData;
            roundData.newRound = false;
            roundData.whatToRead = 2;
            roundData.gameFound = 2;
            roundData.playerId = -1;
            roundData.timeLeft = 3;

            int w = write(games[i].mainPlayerId, &roundData, sizeof(Server_Data));
            if (w == -1)
            {
              perror("Updating Client 1 failed: ");
            }
            w = write(games[i].opponentPlayerId, &roundData, sizeof(Server_Data));
            if (w == -1)
            {
              perror("Updating Client 2 failed: ");
            }
          }

          // Elapsed time.
          std::chrono::duration<float> elap = games[i].startTime - clock_time;

          // On Update
          if (abs(elap.count()) >= games[i].timer && !games[i].isChoiceRound)
          {
            Server_Data roundData;
            roundData.newRound = false;
            roundData.whatToRead = 2;
            roundData.gameFound = 2;
            roundData.playerId = -1;

            // Dirty countdown.
            if (games[i].timer == 1)
            {
              roundData.timeLeft = 2;
            }
            else if (games[i].timer == 2)
            {
              roundData.timeLeft = 1;
            }
            else if (games[i].timer == 3)
            {
              roundData.whatToRead = 6;
              games[i].choiceStartTime = std::chrono::steady_clock::now();
              games[i].isChoiceRound = true;
            }

            int w = write(games[i].mainPlayerId, &roundData, sizeof(Server_Data));
            if (w == -1)
            {
              perror("Updating Client 1 failed: ");
            }
            w = write(games[i].opponentPlayerId, &roundData, sizeof(Server_Data));
            if (w == -1)
            {
              perror("Updating Client 2 failed: ");
            }

            games[i].timer++;
          }

          // On End of Round
          if (games[i].isChoiceRound)
          {
            std::chrono::duration<float> choice_elap = games[i].choiceStartTime - clock_time;

            // Two seconds elapsed. No more input taken.
            if (abs(choice_elap.count()) >= 2.0f)
            {
              games[i].isChoiceRound = false;
              std::cout << "Round over for game with ID: " << i << "\n";
              games[i].resetTime = true;
              games[i].timer = 1;
              games[i].totalRounds++;

              int winner = stoneSicssorBag(games[i].mainPlayerChoice, games[i].opponentPlayerChoice);
              games[i].mainPlayerChoice = -1;
              games[i].opponentPlayerChoice = -1;

              // Announce winners and losers
              if (winner == 0)
              {
                Server_Data tieData;
                tieData.whatToRead = 5;
                tieData.playerId = games[i].mainPlayerId;
                tieData.score = games[i].mainPlayerScore;
                int writeScore = write(games[i].mainPlayerId, &tieData, sizeof(Server_Data));
                if (writeScore == -1)
                {
                  perror("Failed updating end of round to player 1: ");
                }

                tieData.playerId = games[i].opponentPlayerId;
                tieData.score = games[i].opponentPlayerScore;
                writeScore = write(games[i].opponentPlayerId, &tieData, sizeof(Server_Data));
                if (writeScore == -1)
                {
                  perror("Failed updating end of round to player 1: ");
                }
              }
              else
              {
                Server_Data scoreData;
                //std::cout << "Winner of GAME ID: " << i << " is player " << winner << " !\n";
                if (winner == 1) // First player won
                {
                  games[i].mainPlayerScore++;
                  scoreData.whatToRead = 3;
                  scoreData.playerId = games[i].mainPlayerId;
                  scoreData.score = games[i].mainPlayerScore;
                  int writeScore = write(games[i].mainPlayerId, &scoreData, sizeof(Server_Data));
                  if (writeScore == -1)
                  {
                    perror("Failed updating end of round to player 1: ");
                  }

                  scoreData.whatToRead = 4;
                  scoreData.playerId = games[i].opponentPlayerId;
                  scoreData.score = games[i].opponentPlayerScore;
                  writeScore = write(games[i].opponentPlayerId, &scoreData, sizeof(Server_Data));
                  if (writeScore == -1)
                  {
                    perror("Failed updating end of round to player 1: ");
                  }
                }
                else // Second player won
                {
                  games[i].opponentPlayerScore++;
                  scoreData.whatToRead = 4;
                  scoreData.playerId = games[i].mainPlayerId;
                  scoreData.score = games[i].mainPlayerScore;
                  int writeScore = write(games[i].mainPlayerId, &scoreData, sizeof(Server_Data));
                  if (writeScore == -1)
                  {
                    perror("Failed updating end of round to player 1: ");
                  }

                  scoreData.whatToRead = 3;
                  scoreData.playerId = games[i].opponentPlayerId;
                  scoreData.score = games[i].opponentPlayerScore;
                  writeScore = write(games[i].opponentPlayerId, &scoreData, sizeof(Server_Data));
                  if (writeScore == -1)
                  {
                    perror("Failed updating end of round to player 1: ");
                  }
                }
              }
            }
          }
        }
        // write sockets.
        if (FD_ISSET(i, &ready_sockets))
        {
          // A new connection.
          if (i == sockfd)
          {
            socklen_t len = sizeof(cli);
            int client_socket = accept(sockfd, (struct sockaddr *)&cli, &len);
            FD_SET(client_socket, &current_sockets);
            players[client_socket].playerId = client_socket;
            players[client_socket].playerSocket = client_socket;
            std::cout << "New connection found.\nID given: " << client_socket << "\n";
            Server_Data data;
            data.playerId = players[client_socket].playerId;
            int wr = write(client_socket, &data, sizeof(data));
            if (wr == -1)
            {
              std::cout << "Error when reporting ID to player " << i << ".\n";
            }
          }
          else
          {
            Data data;
            int rd = read(i, &data, sizeof(data));
            if (rd == 0)
            {
              printf("User %d has disconnected.\n", i);
              players[i].playerId = -1;
              FD_CLR(i, &current_sockets);
              close(i);
              int opponent = games[players[i].gameID].opponentPlayerId;

              // Message opponent player that game is over.
              if (opponent > 0)
              {
                Server_Data d;
                d.gameFound = false;
                d.whatToRead = 1;

                write(opponent, &d, sizeof(Server_Data));
              }

              game_data newData;
              games[i] = newData;
              continue;
            }
            else
            {

              if (data.wantToJoin < 2)
              {
                // Update client.
                std::cout << "Client: " << data.playerId << " wants to join? - " << data.wantToJoin << "\n";
                players[i].isSearching = data.wantToJoin;
              }

              if(data.wantToSpectate)
              {
                std::cout << "Client " << data.playerId << " wants to spectate? - " << data.wantToSpectate << "\n";

                Server_Data s;
                for(int j = 0; j < 9; j++)
                {
                  for(int k = 0; k < 1024; k++)
                  {
                    if(games[k].nrOfPlayers > 0)
                    {
                      s.games[j].id = k;
                      s.games[j].main = games[k].mainPlayerScore;
                      s.games[j].opp = games[k].opponentPlayerScore;
                    }
                  }
                }
                s.whatToRead = 10;

                write(i, &s, sizeof(Server_Data));
              }

              // Update main players choice.
              if (i == games[players[i].gameID].mainPlayerId)
              {
                games[players[i].gameID].mainPlayerChoice = data.selection;
                std::cout << "Main player choice is: " << data.selection << "\n";

                if (games[players[i].gameID].isChoiceRound)
                {
                  std::chrono::duration<float> elp = games[i].choiceStartTime - clock_time;
                  games[players[i].gameID].mainPlayerRespTime += elp.count();
                }
              }
              // Update opponent players choice.
              else if (i == games[players[i].gameID].opponentPlayerId)
              {
                games[players[i].gameID].opponentPlayerChoice = data.selection;
                std::cout << "Opponent player choice is: " << data.selection << "\n";
                if (games[players[i].gameID].isChoiceRound)
                {
                  std::chrono::duration<float> elp = games[i].choiceStartTime - clock_time;
                  games[players[i].gameID].opponentRespTime += elp.count();
                }
              }
            }
          }
        }
      }
    }
  }

  close(sockfd);
  return EXIT_SUCCESS;
}