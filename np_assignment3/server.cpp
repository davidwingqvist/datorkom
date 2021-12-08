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
#include <iostream>
#include <regex.h>
#include <vector>

// Maximum length of a chat message
#define MAX 257
#define USERLEN 13
#define PACK MAX + USERLEN + 5

struct package_data
{
  char message_owner[USERLEN];
  char message[MAX];
  char message_type[5];
};

// Global values for ease of use.
fd_set current_sockets, ready_sockets;
int sockfd;
struct sockaddr_in cli;

struct client
{
  char nickname[USERLEN] = {};
  bool hasNickname = false;
  int socket = -1;
};

// stored statuses, -1 = closed, 0 = starting up, 1 = accepted
int statuses[FD_SETSIZE] = {-1};

// Returns -1 on fail, 0 on new connection, 1 on message.
int handle_connection(char *data);

int main(int argc, char *argv[])
{
  if (argc != 2)
  {
    printf("No more/less than 1 argument needs to be present. Retry.\n");
    exit(0);
  }

  std::string version_name = "HELLO 1\n";
  char package[PACK];

  regex_t regularexpression;
  int reti;
  int matches = 0;
  regmatch_t items;

  reti = regcomp(&regularexpression, "^[A-Za-z0-9_]+$", REG_EXTENDED);
  if (reti)
  {
    fprintf(stderr, "Could not compile regex.\n");
    exit(1);
  }

  struct addrinfo hints, *servinfo, *p;

  // Divide string into two parts
  char *address = strtok(argv[1], ":");
  char *port = strtok(NULL, "");
  printf("Host %s ", address);
  printf("and port %s\n", port);

  if (!port)
  {
    printf("No port was input, remember that you need to input IP:PORT\n");
    return EXIT_FAILURE;
  }

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

    int enable = 1;
    //setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &enable, sizeof(enable));

    if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
    {
      close(sockfd);
      perror("Listener : Bind");
      exit(EXIT_FAILURE);
      continue;
    }

    break;
  }
  if (p == NULL)
  {
    printf("Couldn't create socket.\n");
    exit(EXIT_FAILURE);
  }

  FD_ZERO(&current_sockets);
  FD_SET(sockfd, &current_sockets);

  freeaddrinfo(servinfo);
  //int flags = fcntl(connfd, F_GETFL, 0);
  //fcntl(connfd, F_SETFL, flags | O_NONBLOCK);

  socklen_t len = sizeof cli;
  struct itimerval alarmTime;
  alarmTime.it_interval.tv_sec = 10;
  alarmTime.it_interval.tv_usec = 10;
  alarmTime.it_value.tv_sec = 10;
  alarmTime.it_value.tv_usec = 10;

  listen(sockfd, 128);

  int FD_MAX = sockfd;
  std::vector<client> clients;
  while (1)
  {
    ready_sockets = current_sockets;
    int sel = select(FD_MAX + 1, &ready_sockets, NULL, NULL, NULL);

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

      for (int i = 0; i < FD_MAX + 1; i++)
      {
        // write sockets.
        if (FD_ISSET(i, &ready_sockets))
        {
          // A new connection.
          if (i == sockfd)
          {
            socklen_t len = sizeof(cli);
            int client_socket = accept(sockfd, (struct sockaddr *)&cli, &len);
            FD_SET(client_socket, &current_sockets);
            std::cout << "New connection found.\n";

            char data[MAX];
            memset(data, 0, sizeof data);
            client c;
            c.socket = client_socket;
            clients.push_back(c);

            std::cout << "Sending over [HELLO 1] to client.\n";
            int wr = write(client_socket, version_name.c_str(), version_name.length());
            if (wr == -1)
            {
              perror("Write to joined client : ");
            }
            if (client_socket > FD_MAX)
            {
              FD_MAX = client_socket;
            }
          }
          else
          {
            char data[1024];
            memset(data, 0, sizeof data);
            int rd = read(i, data, sizeof(data));
            if (rd == 0)
            {
              // A 0 return from read means connection has been discontinued.
              printf("User has disconnected.\n");
              FD_CLR(i, &current_sockets);

              // REmove
              for (int j = 0; j < clients.size(); j++)
              {
                if (i == clients[j].socket)
                {
                  clients.erase(clients.begin() + j);
                  break;
                }
              }
              close(i);
              continue;
            }
            char command[50];
            char nick[14];
            char message[1024];
            char buffer[1024];

            std::string stringData(data);

            while (rd > 0)
            {
              memset(command, 0, 50);
              memset(nick, 0, 14);
              memset(message, 0, 1024);
              memset(buffer, 0, 1024);

              int s = sscanf(stringData.c_str(), "%s %[^\n]", command, message);

              if (strcmp(command, "MSG") == 0)
              {
                int bytesHandled = strlen(command) + strlen(message) + 2;
                if (bytesHandled > stringData.length())
                  break;
                //printf("%s %s\n", command, message);
                stringData = stringData.substr(bytesHandled, rd - bytesHandled);

                //printf("Before: %d\n", rd);
                rd -= bytesHandled;
                //printf("After: %d\n", rd);
                bool hasnickname = true;
                for (int j = 0; j < clients.size(); j++)
                {
                  if (clients[j].socket == i)
                  {
                    if (!clients[j].hasNickname)
                    {
                      write(clients[j].socket, "ERROR no nick set\n", strlen("ERROR no nick set\n"));
                      FD_CLR(i, &ready_sockets);
                      hasnickname = false;
                      break;
                    }
                    strncpy(nick, clients[j].nickname, 12);
                    sprintf(buffer, "%s %s %s\n", command, nick, message);
                    //printf("%s", buffer);
                    break;
                  }
                }

                // NO NICKNAME! DONT SEND MESSAGS
                if (!hasnickname)
                {
                  continue;
                }

                //std::cout << "Sending: " << buffer << " \n";
                //std::cout << "CLient size: " << clients.size() << "\n";
                
                // Send the package to all clients.
                for (int j = 0; j < clients.size(); j++)
                {
                  // Dont send any packages to any clients with no nicknames.
                  if(clients[j].hasNickname)
                  {
                    int wr = write(clients[j].socket, buffer, strlen(buffer));
                    if (wr == -1)
                    {
                      perror("Write to client : ");
                    }
                  }
                }
              }
              else // If MSG is not present, Then skip onto NICK
                break;
            }
            if (strcmp(command, "NICK") == 0 && rd > 0)
            {
              for (int j = 0; j < clients.size(); j++)
              {
                if (i == clients[j].socket)
                {
                  std::string acceptance;
                  std::string newName(message);

                  if (newName.length() <= 12)
                  {
                    reti = regexec(&regularexpression, newName.c_str(), matches, &items, 0);
                    if (!reti)
                    {
                      printf("(Nickname)%s is accepted.\n", newName.c_str());
                      acceptance = "OK\n";
                      strncpy(clients[j].nickname, newName.c_str(), 12);
                      clients[j].hasNickname = true;
                    }
                    else
                    {
                      printf("(Nickname)%s is NOT accepted.\n", newName.c_str());
                      acceptance = "ERROR\n";
                    }
                  }
                  else
                  {
                    printf("(Nickname)%s is TOO LONG.\n", newName.c_str());
                    acceptance = "ERROR\n";
                  }

                  int acc = write(i, acceptance.c_str(), acceptance.length());
                  if (acc == -1)
                    perror("Error sending acceptance : ");
                }
              }
            }
            else if (rd > 0)
            {
              write(i, "ERROR MALFORMED COMMAND\n", strlen("ERROR MALFORMED COMMAND\n"));
            }
          }

          FD_CLR(i, &ready_sockets);
        }
      }
    }
  }

  regfree(&regularexpression);
  close(sockfd);
  return EXIT_SUCCESS;
}

int handle_connection(char *data)
{
  std::string compare(data);
  //std::cout << "String to compare: " << compare << "\n";
  if (compare.find("MSG") <= 3)
  {
    return 1;
  }
  if (compare.find("NICK") <= 3)
  {
    return -1;
  }

  return -2;
}