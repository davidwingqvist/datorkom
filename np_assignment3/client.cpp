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
#include <unistd.h>
#include <vector>
#include <iostream>
#include <string>

#define MAX 255
#define USERLEN 12
#define PACK MAX + USERLEN + 10
#define STDIN 0

struct package_data
{
  char message_type[5];
  char message_owner[USERLEN];
  char message[MAX];
};

fd_set current_sockets, ready_sockets;

int main(int argc, char *argv[]){
  
  	/* Do magic */
	if(argc != 3)
  {
    printf("ONLY 3 ARGUMENTS ARE ACCEPTED\n PROGRAM IP:PORT USERNAME\n");
    exit(0);
  }

  char servbuf[MAX];
  char user_name[USERLEN];
  
  char package[PACK];

  strcpy(user_name, argv[2]);

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

  /*
  strcat(package, "MSG");
  strcat(package, user_name);
  strcat(package, message);
  */

  

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
            char message[MAX];
            memset(message, 0 , sizeof message);
            fgets(message, 255, stdin);

            package_data p1;
            strcpy(p1.message_type, "MSG");
            strcpy(p1.message_owner, user_name);
            strcpy(p1.message, message);

            int wr = write(sockfd, &p1, sizeof(package_data));
            if(wr == -1)
              perror("Error write : ");
          }
          // Read for messages.
          else if(i == sockfd)
          {
            package_data data;
            int r = read(sockfd, &data, sizeof(package_data));
            if(r > 0)
            {
              std::cout << data.message_owner << ": " << data.message;
            }
            else
            {
              // Disconnect.
              perror("Error read : ");
              close(sockfd);
              exit(EXIT_FAILURE);
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

