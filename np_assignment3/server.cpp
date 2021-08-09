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

// Maximum length of a chat message
#define MAX 255

int main(int argc, char *argv[])
{
	if(argc != 2)
  	{
    	printf("No more/less than 1 argument needs to be present. Retry.\n");
    	exit(0);
  	}

  std::string version_name = "HELLO 1";

  /* Do more magic */
  int connfd;
  struct sockaddr_in servaddr, cli;

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
  //int flags = fcntl(connfd, F_GETFL, 0);
	//fcntl(connfd, F_SETFL, flags | O_NONBLOCK);

  socklen_t len = sizeof cli;
  struct itimerval alarmTime;
  alarmTime.it_interval.tv_sec=10;
  alarmTime.it_interval.tv_usec=10;
  alarmTime.it_value.tv_sec=10;
  alarmTime.it_value.tv_usec=10;
  
while(1)
{
	printf("Waiting for next connection...\n");
	listen(sockfd, 5); // Listening for connections
  
	len = sizeof(cli);
	connfd = accept(sockfd, (struct sockaddr*)&cli, &len); // Accepted

  memset(msg_buf, 0, sizeof msg_buf);
  int size = read(connfd, msg_buf, sizeof(msg_buf));
  printf("Message: %s\n", msg_buf);
  
	//Write out supported protocols
  memset(msg_buf, 0, sizeof(msg_buf));
	write(connfd, msg_buf, strlen(msg_buf));

	close(connfd);
}

  return 0;
}
