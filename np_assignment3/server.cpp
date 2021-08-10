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
#include <sys/time.h>
#include <string>
#include <iostream>

// Maximum length of a chat message
#define MAX 255
#define USERLEN 12
#define PACK MAX + USERLEN + 5

struct package_data
{
  char message_type[4];
  char message_owner[USERLEN];
  char message[MAX];
};

// Global values for ease of use.
fd_set current_sockets, ready_sockets;
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
  char* adress = strtok(argv[1], ":");
  char* port = strtok(NULL, "");
  printf("Host %s ", adress);
  printf("and port %s\n", port);

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_CANONNAME;
  
  int rv;
  if((rv = getaddrinfo(adress, port, &hints, &servinfo)) != 0)
  {
      perror("Adress info");
      exit(0);
  }

  
  for(p = servinfo; p != NULL; p = p->ai_next)
  {
      if((sockfd = socket(p->ai_family, p->ai_socktype,
                        p->ai_protocol)) == -1)
      {
        perror("Listener : Socket");
        continue;
      }

      if(bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
      {
        close(sockfd);
        perror("Listener : Bind");
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
  int sel = select(FD_SETSIZE, &ready_sockets, NULL, NULL, NULL);

  switch (sel)
  {
    case -1: 
    //std::cout << "Error with connection.\n";

    break;
    case 0:
    // Timeout.
    std::cout << "Connection timed out.\n";

    break;
    default:

    for(int i = 0; i < FD_SETSIZE; i++)
    {
      if(FD_ISSET(i, &ready_sockets))
      {
        // A new connection.
        if(i == sockfd)
        {
          socklen_t len = sizeof(cli);
          int client_socket = accept(sockfd, (struct sockaddr*)&cli, &len);
          FD_SET(client_socket, &current_sockets);
          std::cout << "New connection found.\n";
        }
        else
        {
          package_data data;
          memset(&data, 0, sizeof(package_data));
          read(i, &data, sizeof(package_data));
          int result = handle_connection(&data);

          // Handle.
          if(result >= 0)
          {
            std::cout << data.message_type << " " << data.message_owner << " " << data.message;
          }
        }
      }
    }

    break;
  }
  }

  return 0;
}

int handle_connection(package_data* data)
{
  if(strcmp(data->message_type, "MSG") != NULL)
  {
    return 1;
  }
  else if (strcmp(data->message_type, "JOIN") != NULL)
  {
    return 0;
  }
  else if (strcmp(data->message_type, "NICK") != NULL)
  {
    return 2;
  }
  return -1;
}