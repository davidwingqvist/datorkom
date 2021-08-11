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

// Maximum length of a chat message
#define MAX 255
#define USERLEN 12
#define PACK MAX + USERLEN + 5

struct package_data
{
  char message_owner[USERLEN];
  char message[MAX];
  char message_type[5];
};

// Global values for ease of use.
fd_set current_sockets, ready_sockets, handle_sockets;
int sockfd;
struct sockaddr_in cli;

// stored nicknames
std::string nick_names[FD_SETSIZE] = {"NON"};

// stored statuses, -1 = closed, 0 = starting up, 1 = accepted
int statuses[FD_SETSIZE] = { -1 };


// Returns -1 on fail, 0 on new connection, 1 on message.
int handle_connection(char* data);

int main(int argc, char *argv[])
{
	if(argc != 2)
  	{
    	printf("No more/less than 1 argument needs to be present. Retry.\n");
    	exit(0);
  	}

  std::string version_name = "HELLO 1";
  char package[PACK];

  regex_t regularexpression;
  int reti;
  int matches = 0;
  regmatch_t items;
  
  reti=regcomp(&regularexpression, "[A-Za-z0-9]", REG_EXTENDED);
  if(reti){
    fprintf(stderr, "Could not compile regex.\n");
    exit(1);
  }
  
  struct addrinfo hints, *servinfo, *p;
  

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

  listen(sockfd, 128);
  
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
          std::cout << "New connection found.\n";

          char data[MAX];
          memset(data, 0, sizeof data);
          
          strncpy(data, "Hello 1\n", MAX);
          int wr = write(client_socket, &data, strlen(data));
          if(wr == -1)
          {
            perror("Write to joined client : ");
          }
          statuses[client_socket] = 0;
        }
        else
        {
          char data[MAX];
          memset(data, 0, sizeof data);
          int rd = read(i, &data, sizeof(data));
          if(rd == 0)
          {
            // A 0 return from read means connection has been discontinued.
            printf("User has disconnected.\n");
            FD_CLR(i, &current_sockets);
            statuses[i] = -1;
            nick_names[i] = "NON";
            close(i);
            continue;
          }
          int result = handle_connection(data);
          //std::cout << "Result: " << result << "\n";

          // Handle.
          if(result == 1 && statuses[i] == 1 && rd > 0)
          {
            std::string pack(data);
            std::string mess = "NON";

            if(pack.length() > 0)
              mess = pack.substr(4, pack.length() - 1);

            memset(data, 0, sizeof data);

            // Divide it up so we can format it.
            strcpy(data, "MSG ");
            strcat(data, nick_names[i].c_str());
            strcat(data, " ");
            strcat(data, mess.c_str());

            //std::cout << data;

            // Send the package to all clients.
            for(int j = 0; j < FD_SETSIZE; j++)
            {
              // Write sockets.
              if(FD_ISSET(j, &handle_sockets) && statuses[j] == 1)
              {
                int wr = write(j, &data, strlen(data));
                if(wr == -1)
                {
                  perror("Write to client : ");
                }
              }
            }
          }
          else if (result == -1) // Handle the nickname call.
          {
            char acceptance[MAX];
            memset(acceptance, 0, sizeof acceptance);


              if(strlen(data)<12){
              reti=regexec(&regularexpression, data, matches, &items,0);
              if(!reti){
    	          printf("Nickname is accepted.\n");
                statuses[i] = 1;
                nick_names[i] = std::string(data);
                strcpy(acceptance, "OK");
              } else {
	              printf("Nickname is NOT accepted.\n");
                statuses[i] = 0;
                nick_names[i] = "NON";
                strcpy(acceptance, "ERROR");
              }
              } else {
                printf("Nickname is TOO LONG.\n");
                statuses[i] = 0;
                nick_names[i] = "NON";
                strcpy(acceptance, "ERROR");
              }

            int acc = write(i, acceptance, sizeof acceptance);
            if(acc == -1)
              perror("Error sending acceptance : ");
          }
        }
      }

      
    }

    //break;
  }
  }

  regfree(&regularexpression);
  close(sockfd);
  return EXIT_SUCCESS;
}

int handle_connection(char* data)
{
  std::string compare(data);
  //std::cout << "String to compare: " << compare << "\n";
  if(compare.find("MSG") <= 3)
  {
    return 1;
  }

  return -1;
}