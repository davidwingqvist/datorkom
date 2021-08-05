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
#include <sys/select.h>
#define MAX 100

// Included to get the support library
#include <calcLib.h>

using namespace std;

int initCalcLib(void);
int randomInt(void);
double randomFloat(void);
char* randomType(void);

int main(int argc, char *argv[])
{
	if(argc != 2)
  	{
    	printf("No more/less than 1 argument needs to be present. Retry.\n");
    	exit(0);
  	}

  /* Do more magic */
  initCalcLib();
  int connfd;
  socklen_t len;
  struct sockaddr_in servaddr, cli;
  char serv_buf[MAX] = "TEXT TCP 1.0\n\n";
  char their_buf[MAX];
  
  char calc[MAX];
  char cli_calc[MAX];
  char cval[MAX];
  int i_result = -999999;
  double d_result = -999999;
  int var = -999999;
  double fval1, fval2 = -999999;
  char* type;
  
  int val1 = -999999, val2 = -999999;
  
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

  struct timeval time;
  time.tv_sec = 5;
  time.tv_usec = 5;
  int s = setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&time, sizeof(time));
  if(s < 0)
  {
	  perror("Set Socket Opts");
  }

  freeaddrinfo(servinfo);
  
while(1)
{
	printf("Waiting for next connection...\n");
	listen(sockfd, 5); // Listening for connections
    	
	len = sizeof(cli);
	connfd = accept(sockfd, (struct sockaddr*)&cli, &len); // Accepted
		
	//Write out supported protocols
	write(connfd, serv_buf, strlen(serv_buf));
	
	memset(&their_buf, 0, sizeof(their_buf));

	if(read(connfd, cli_calc, sizeof(cli_calc)) < 0)
	{
		printf("Error (Timeout.)\n");
		char error_buf[MAX] = "ERROR TO\n";
		write(connfd, error_buf, strlen(error_buf));
		close(connfd);
		continue;
	}
		
	if(strstr(cli_calc, "OK") != NULL)
	{
		memset(&cval, 0, sizeof(cval));
		memset(&type, 0, sizeof(type));
		val1 = randomInt();
		val2 = randomInt();
		type = randomType();
		fval1 = randomFloat();
		fval2 = randomFloat();

		if(strcmp(type, "add") == 0)
		{
			i_result = (int)(val1 + val2);
			var = 1;
		}
		if(strcmp(type, "div") == 0)
		{
			i_result = (int)(val1 / val2);
			var = 1;
		}
		if(strcmp(type, "mul") == 0)
		{
			i_result = (int)(val1 * val2);
			var = 1;
		}
		if(strcmp(type, "sub") == 0)
		{
			i_result = (int)(val1 - val2);
			var = 1;
		}
		
		if(strcmp(type, "fadd") == 0)
		{
			d_result = fval1 + fval2;
			var = 2;
		}
		if(strcmp(type, "fdiv") == 0)
		{
			d_result = (fval1 / fval2);
			var = 2;
		}
		if(strcmp(type, "fmul") == 0)
		{
			d_result = fval1 * fval2;
			var = 2;
		}
		if(strcmp(type, "fsub") == 0)
		{
			d_result = fval1 - fval2;
			var = 2;
		}
		
		if(var == 1)
		{
			sprintf(cval, "%s %d %d\n", type, val1, val2);
			sprintf(calc, "%d", i_result);
		}
		else if(var == 2)
		{
			sprintf(cval, "%s %8.8g %8.8g\n", type, fval1, fval2);
			sprintf(calc, "%8.8g", d_result);
		}
		
		printf("Assignment: %s", cval);
		write(connfd, cval, strlen(cval));
		
		memset(&cli_calc, 0, sizeof(cli_calc));
		
		if(read(connfd, cli_calc, sizeof(cli_calc)) < 0)
		{
			printf("Error (Timeout.)\n");
			char error_buf[MAX] = "ERROR TO\n";
			write(connfd, error_buf, strlen(error_buf));
			close(connfd);
			continue;
		}

		if(var == 1)
		{
			int clientCal = atoi(cli_calc);

			int d = abs(i_result - clientCal);
			//printf("%d", d);
			if( d < 0.0001f)
			{
				printf("OK\n");
				char end_buf[MAX] = "OK\n";
				write(connfd, end_buf, strlen(end_buf));
			}
			else
			{
				int serverCal = i_result;
				printf("Error (%d, %d not the same.)\n", clientCal, serverCal);
				char error_buf[MAX] = "Error\n";
				write(connfd, error_buf, strlen(error_buf));
			}
		}
		else
		{
			double serverCal = strtod(cli_calc, NULL);
			double clientCal = d_result;

			double d = abs((double)serverCal - (double)clientCal);
			if( d < 0.0001f)
			{
				printf("OK\n");
				char end_buf[MAX] = "OK\n";
				write(connfd, end_buf, strlen(end_buf));
			}
			else
			{
				printf("Error (%8.8g, %8.8g not the same.)\n", clientCal, serverCal);
				char error_buf[MAX] = "Error\n";
				write(connfd, error_buf, strlen(error_buf));
			}
		}
		
		
		
	}
	else
	{
		printf("Error (Not supported.)\n");
		char error_buf[MAX] = "Error\n";
		write(connfd, error_buf, strlen(error_buf));
	}
		
	close(connfd);
}

  return 0;
}
