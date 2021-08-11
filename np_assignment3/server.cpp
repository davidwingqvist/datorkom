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

// Maximum length of a chat message
#define MAX 255
#define USERLEN 12
#define PACK MAX + USERLEN + 5

struct package_data
{
  char message_type[5];
  char message_owner[USERLEN];
  char message[MAX];
};

// Global values for ease of use.
fd_set current_sockets, ready_sockets, handle_sockets;
int sockfd;
struct sockaddr_in cli;


// Returns -1 on fail, 0 on new connection, 1 on message.
int handle_connection(package_data* data);

int main(int argc, char *argv[])
{
	if(argc != 2)
  	{
    	printf("No more/less than 1 argument needs to be present. Retry.\n");
    	exit(0);
  	}

  std::string version_name = "HELLO 1";
  char package[PACK];

  /* Do more magic */
  //int connfd;
  

  // Constantly pointing to text "TEXT TCP 1.0"
  char msg_buf[MAX];
  
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

          package_data data;
          strcpy(data.message, "Hello 1\n\n");
          strcpy(data.message_owner, "SERVER");
          strcpy(data.message_type, "JOIN");
          int wr = write(client_socket, &data, sizeof(package_data));
          if(wr == -1)
          {
            perror("Write to joined client : ");
          }
        }
        else
        {
          package_data data;
          int rd = read(i, &data, sizeof(package_data));
          if(rd == 0)
          {
            // A 0 return from read means connection has been discontinued.
            printf("User has disconnected.\n");
            FD_CLR(i, &current_sockets);
            close(i);
            break;
          }
          int result = handle_connection(&data);

          // Handle.
          if(result == 1)
          {
            std::cout << data.message_type << " " << data.message_owner << " " << data.message;
            for(int j = 0; j < FD_SETSIZE; j++)
            {
              // Write sockets.
              if(FD_ISSET(j, &handle_sockets) && j != sockfd && j != i)
              {
                int wr = write(j, &data, sizeof(package_data));
                if(wr == -1)
                {
                  perror("Write to client : ");
                }
              }
            }
          }
        }
      }

      
    }

    break;
  }
  }

  return EXIT_SUCCESS;
}

int handle_connection(package_data* data)
{
  if(strcmp(data->message_type, "MSG") == 0)
  {
    return 1;
  }
  else if (strcmp(data->message_type, "JOIN") == 0)
  {
    return 0;
  }
  else if (strcmp(data->message_type, "NICK") == 0)
  {
    return 2;
  }
  return -1;
}