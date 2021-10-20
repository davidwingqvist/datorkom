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
#include <chrono>
#include <termios.h>
#include <stdbool.h>
#include <sys/ioctl.h>

#define STDIN 0
#define MAX_VIEWER 128
#define MAX_GAMES 9

bool canMakeMove = false;
bool isSpectator = false;

bool fastFix = false;

void MainMenu();
void ChooseGameMenu();
void SearchingForGame();
void selectMoveScreen();

struct _player
{
  int id = -1;
  float time = 2001.0f;
};

struct _highscore
{
  _player scores[5];
};

struct compressed_game
{
  int main = 0;
  int opp = 0;
  int id = -1;
  int active = -1;
};

struct availableGame
{
  int games[FD_SETSIZE] = {-1};
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

struct Server_Data
{
  // WhatToRead = -1
  int playerId = -1;

  // Specifies what to read in this package.
  int whatToRead = -1;

  // WhatToRead = 0
  bool gameFound = false;

  // WhatToRead = 1
  int timeLeft = 0;

  bool newRound = false;

  int score = 0;

  compressed_game games[MAX_GAMES];
  _highscore scores;
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

  int wantHighScore = false;
};

Data SendDataToServer(int id, int isSpectator, int selection);

/*
  Different states mandate what kind of reaction a input will cause.
  0 - Main menu.
  1 - Choosing game to join.
  2 - Choosing game to watch.
  3 - Inside a game.
  4 - Watching a game.
*/
struct state
{
  int playerId = -1;
  int current_state = 0;
  bool isSpectator = false;
} client_state;

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

Data test;
//availableGame games;
compressed_game games[MAX_GAMES];

fd_set current_sockets, ready_sockets;
int main(int argc, char *argv[])
{
  /* Do magic */
  if (argc != 2)
  {
    printf("ONLY 2 ARGUMENTS ARE ACCEPTED\n PROGRAM IP:PORT\n");
    exit(0);
  }

  struct addrinfo hints, *servinfo, *p;
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

  int sockfd;
  for (p = servinfo; p != NULL; p = p->ai_next)
  {
    if ((sockfd = socket(p->ai_family, p->ai_socktype,
                         p->ai_protocol)) == -1)
    {
      perror("Listener : Socket");
      close(sockfd);
      exit(EXIT_FAILURE);
      continue;
    }

    if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1)
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

  while (1)
  {
    ready_sockets = current_sockets;
    int sel = select(FD_SETSIZE, &ready_sockets, NULL, NULL, NULL);

    switch (sel)
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

      for (int i = 0; i < FD_SETSIZE; i++)
      {
        if (FD_ISSET(i, &ready_sockets))
        {
          // input
          if (i == 0)
          {
            Data data_sel;
            int wr;
            char input[1];
            // Hide echo
            //system("stty -echo");
            std::cin >> input;

            switch (client_state.current_state)
            {
            case 0:
              if (strcmp(input, "0") == NULL)
              {
                client_state.current_state = 1;
                client_state.isSpectator = false;
                SearchingForGame();

                // Setting up join request data package.
                Data data;
                data.playerId = client_state.playerId;
                data.wantToJoin = 1;
                data.isSpectator = false;
                data.selection = -1;

                int wr = write(sockfd, &data, sizeof(data));
                if (wr == -1)
                  std::cout << "Sending Join request to server failed.\n";
              }
              else if (strcmp(input, "1") == NULL)
              {
                client_state.current_state = 2;
                //client_state.isSpectator = true;
                ChooseGameMenu();

                Data d;
                d.wantToSpectate = true;
                d.selection = 999;
                d.playerId = client_state.playerId;
                write(sockfd, &d, sizeof(Data));
              }
              else if (strcmp(input, "2") == NULL)
              {
                close(sockfd);
                exit(EXIT_SUCCESS);
              }
              else if (strcmp(input, "3") == NULL)
              {
                client_state.current_state = 5;
                Data dp;
                dp.wantHighScore = true;
                dp.wantToJoin = false;
                dp.playerId = client_state.playerId;
                write(sockfd, &dp, sizeof(Data));
              }
              break;
            case 1:
              break;
            case 2:
              // Spectate game scene.
              for (int i = 0; i < 9; i++)
              {
                if (strcmp(input, std::string(std::to_string(i)).c_str()) == NULL)
                {
                  if (games[i].active != -1)
                  {
                    Data dl;
                    dl.playerId = client_state.playerId;
                    dl.wantToSpectate = true;
                    dl.isSpectator = true;
                    dl.selection = games[i].id;

                    write(sockfd, &dl, sizeof(Data));
                    client_state.isSpectator = true;
                    client_state.current_state = 4;
                  }
                }
              }
              if (strcmp(input, "r") == NULL)
              {
                client_state.current_state = 0;
                MainMenu();
              }

              break;
            case 3:

              if (canMakeMove)
              {
                if (strcmp(input, "1") == NULL)
                {
                  data_sel.selection = 0;
                  data_sel.playerId = client_state.playerId;
                  data_sel.isSpectator = false;
                  data_sel.wantToJoin = 2;
                  canMakeMove = false;
                }
                else if (strcmp(input, "2") == NULL)
                {
                  data_sel.selection = 1;
                  data_sel.playerId = client_state.playerId;
                  data_sel.isSpectator = false;
                  data_sel.wantToJoin = 2;
                  canMakeMove = false;
                }
                else if (strcmp(input, "3") == NULL)
                {
                  data_sel.selection = 2;
                  data_sel.playerId = client_state.playerId;
                  data_sel.isSpectator = false;
                  data_sel.wantToJoin = 2;
                  canMakeMove = false;
                }
                else
                {
                  data_sel.selection = -1;
                  data_sel.playerId = client_state.playerId;
                  data_sel.isSpectator = false;
                  data_sel.wantToJoin = 2;
                  canMakeMove = false;
                }
              }

              wr = write(sockfd, &data_sel, sizeof(Data));
              if (wr == -1)
              {
                perror("Failed to Update selection to server: ");
              }
              break;
            case 4:
              if (strcmp(input, "r") == NULL)
              {
                client_state.current_state = 0;
                client_state.isSpectator = false;
                MainMenu();

                Data stopSpec;
                stopSpec.selection = 500;

                write(sockfd, &stopSpec, sizeof(Data));
              }
              break;
            case 5:
              if (strcmp(input, "r") == NULL)
              {
                std::cout << "Returning to Menu...\n";
                client_state.current_state = 0;
                MainMenu();
              }
              break;
            }
          }
          // Read for messages.
          else if (i == sockfd)
          {
            Server_Data message;
            int r = read(sockfd, &message, sizeof(message));
            if (r > 0)
            {
              switch (message.whatToRead)
              {
              case -1:
                client_state.playerId = message.playerId;
                std::cout << "Current ID: " << client_state.playerId << ".\n";
                break;
              case 0:
                //std::cout << "Game has been found!\n";
                client_state.current_state = 3;
                break;
              case -2:
                MainMenu();
                client_state.current_state = 0;
                break;
              case 2:
                std::cout << "Seconds Left: " << message.timeLeft << "\n";
                canMakeMove = false;
                break;
              case 3:
                std::cout << "You won the Round!\nYour score: " << message.score << "\n";
                fastFix = false;
                break;
              case 4:
                std::cout << "You lost the Round!\nYour score: " << message.score << "\n";
                fastFix = true;
              case 5:
                if (!fastFix)
                {
                  std::cout << "This round was a TIE!\n";
                  std::cout << "Your score: " << message.score << "\n";
                }
                fastFix = !fastFix;
                break;
              case 6:
                // Make a move.
                selectMoveScreen();
                canMakeMove = true;
                break;
              case 7:
                std::cout << "You lost the game!\n";
                MainMenu();
                client_state.current_state = 0;
                break;
              case 8:
                std::cout << "You won the game!\n";
                MainMenu();
                client_state.current_state = 0;
                break;
              case 9:
                std::cout << "Highscore board!\n";
                for (int i = 0; i < 5; i++)
                {
                  if (message.scores.scores[i].id == -1)
                    std::cout << "Empty spot!\n";
                  else
                    std::cout << "ID: " << message.scores.scores[i].id << " with " << message.scores.scores[i].time << " ms reaction time!\n";
                }
                std::cout << "Press [r] and enter to return.\n";
                break;
              case 10:
                client_state.current_state = 2;
                std::cout << "Simply press 'r' and enter to return to main menu.\n";
                //print out all games.
                for (int i = 0; i < 9; i++)
                {
                  if (message.games[i].active == 1)
                  {
                    std::cout << "Game " << i << ": Player 1 - " << message.games[i].main << " against ";
                    std::cout << "Player 2 - " << message.games[i].opp << "\n";
                  }
                  games[i] = message.games[i];
                }
                break;
              case 11:
                std::cout << "Time left for players: " << message.timeLeft << "\n";
                break;
              case 12:
                std::cout << "Player 1 score: " << message.scores.scores[0].id << "\n";
                std::cout << "Player 2 score: " << message.scores.scores[1].id << "\n";
                break;
              case 13:
                MainMenu();
                client_state.current_state = 0;
                break;
              }
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

  std::cout << "[0] - Join a Game.\n[1] - Spectate a Game.\n[2] - Quit Game\n[3] - Highscore List\n";
}

void ChooseGameMenu()
{
  std::cout << "Please choosing one of the following available games.\n";
  std::cout << "TIP - To watch a game simply press the associated number and confirm with enter.\n";

  // std::cout << [ << i << ] << " Ongoing: " << player1.score << " - " << player2.score << "\n";
}

void selectMoveScreen()
{
  std::cout << "Please choose one of the following choices before the time is up!\n";
  std::cout << "Leaving it blank or inputting anything other than 1, 2 or 3 will forfeit this round.\n";
  std::cout << "[1] - Stone\n";
  std::cout << "[2] - Scissor\n";
  std::cout << "[3] - Paper\n";
}

void SearchingForGame()
{
  std::cout << "You are now currently searching for a game...\nPlease have patients while the server matches you up with a player.\n";
}

Data SendDataToServer(int id, int isSpectator, int selection)
{
  Data data;
  data.playerId = id;
  data.isSpectator = isSpectator;
  data.selection = selection;
  return data;
}