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

// Maximum length of a chat message
#define MAX 255

int main(int argc, char *argv[])
{
	if(argc != 2)
  	{
    	printf("No more/less than 1 argument needs to be present. Retry.\n");
    	exit(0);
  	}

  /* Do more magic */
  int connfd;
  socklen_t len;
  struct sockaddr_in servaddr, cli;
  char serv_buf[MAX] = "TEXT TCP 1.0\n\n";
  char their_buf[MAX];
  
  char calc[MAX];
  char cli_calc[MAX];
  char cval[MAX];
  int i_result;
  double d_result;
  int var = 0;
  double fval1, fval2;
  char* type;
  
  int val1, val2;
  
 struct addrinfo hints, *servinfo, *p;

  int amount_of_units = 0;

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

  int sockfd;
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

  freeaddrinfo(servinfo);
  
while(1)
{
	printf("Waiting for next connection...\n");
	listen(sockfd, 5); // Listening for connections
  
	len = sizeof(cli);
	connfd = accept(sockfd, (struct sockaddr*)&cli, &len); // Accepted

	int flags = fcntl(connfd, F_GETFL, 0);
	fcntl(connfd, F_SETFL, flags | O_NONBLOCK);
  
	//Write out supported protocols
	write(connfd, serv_buf, strlen(serv_buf));

	close(connfd);
}

  return 0;
}
