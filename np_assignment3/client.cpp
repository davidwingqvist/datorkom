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

int main(int argc, char *argv[]){
  
  	/* Do magic */
	if(argc != 3)
  {
    printf("ONLY 3 ARGUMENTS ARE ACCEPTED\n PROGRAM IP:PORT USERNAME\n");
    exit(0);
  }
  
  std::string version_name = "HELLO 1";

  char servbuf[MAX];
  std::string user_name = argv[2];

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
  freeaddrinfo(servinfo);

  while(1)
  {
    // main loop.
    
  }

	memset(servbuf, 0, sizeof(servbuf));
  strcpy(servbuf, argv[2]);
  std::cout << servbuf << "\n";

	write(sockfd, servbuf, strlen(servbuf));
  printf("%s\n", servbuf);
 
  close(sockfd);
  return 0;

}

