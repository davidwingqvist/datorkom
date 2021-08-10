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
  char message[MAX];
  char package[PACK];

  strcpy(user_name, argv[2]);

  struct addrinfo hints, *servinfo, *p;
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

  int sockfd;
  for(p = servinfo; p != NULL; p = p->ai_next)
  {
      if((sockfd = socket(p->ai_family, p->ai_socktype,
                        p->ai_protocol)) == -1)
      {
        perror("Listener : Socket");
        continue;
      }

      if(connect(sockfd, p->ai_addr, p->ai_addrlen) == -1)
      {
        close(sockfd);
        perror("Listener : Connect");
        continue;
      }

      break;
  }

  FD_ZERO(&current_sockets);
  FD_SET(sockfd, &current_sockets);
  FD_SET(0, &current_sockets);

  freeaddrinfo(servinfo);

  fgets(message, 255, stdin);
  strcat(package, "MSG");
  strcat(package, user_name);
  strcat(package, message);

  package_data p1;
  strcat(p1.message_type, "MSG");
  strcat(p1.message_owner, user_name);
  strcat(p1.message, message);

  while(1)
  {
    ready_sockets = current_sockets;

    int sel = select(FD_SETSIZE, &ready_sockets, NULL, NULL, NULL);
    switch(sel)
    {
      case 0:
      break;
      case -1:
      break;
      default:

      for(int i = 0; i < FD_SETSIZE; i++)
      {
        if(FD_ISSET(i, &ready_sockets))
        {
          // input
          if(i == 0)
          {
            write(sockfd, &p1, sizeof(package_data));
            std::cout << p1.message_type << " " << p1.message_owner << " " << p1.message;
            FD_ZERO(i, &ready_sockets);
          }
        }
      }

      break;
    }
	  
  }
 
  close(sockfd);
  return 0;

}

