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

#define MAX 100


// Included to get the support library
#include <calcLib.h>

int main(int argc, char *argv[]){
  
  	/* Do magic */
	if(argc != 2)
  	{
    	printf("No more/less than 1 argument needs to be present. Retry.\n");
    	exit(0);
  	}

  int sockfd;
  struct sockaddr_in servaddr;
  char servbuf[MAX];
  int val1, val2;
  double fval1, fval2;
  int i_result;
  double d_result;
  char res[MAX];
  char type_buf[MAX];
  
  // Reset the memory positions of all values to lower chance of crap values
  memset(&val1, 0, sizeof(val1));
  memset(&val2, 0, sizeof(val2));
  memset(&fval1, 0, sizeof(fval1));
  memset(&fval2, 0, sizeof(fval2));
  memset(&i_result, 0, sizeof(i_result));
  memset(&d_result, 0, sizeof(d_result));
  memset(&type_buf, 0, sizeof(type_buf));
  
  if((sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
  {
	  perror("Socket");
  }

  // Divide string into two parts 
  char* adress = strtok(argv[1], ":");
  char* port = strtok(NULL, "");
  printf("Host %s ", adress);
  printf("and port %s\n", port);
  servaddr.sin_family = AF_INET; 
  servaddr.sin_addr.s_addr = inet_addr(adress); 
  servaddr.sin_port = htons(atoi(port)); 
  
  if((connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr))) < 0)
  {
	  perror("Connect");
  }
	
	// Reset memory each time before we read into it
	memset(&servbuf, 0, sizeof(servbuf));
	read(sockfd, &servbuf, sizeof(servbuf));

	// Our supported protocol.
	char protocol[] = "TEXT TCP 1.0";

	// Tries to find substring inside a bigger string, returns pointer to it.
	if(strstr(servbuf, protocol) != NULL)
	{
		char clibuf[MAX] = "OK\n";
		write(sockfd, clibuf, strlen(clibuf));

		memset(&servbuf, 0, sizeof(servbuf));
		read(sockfd, &servbuf, sizeof(servbuf));
		printf("Assignment %s",servbuf);

		if(strchr(servbuf, 'f') != NULL)
		{			
			sscanf(servbuf, "%s %lf %lf\n", type_buf, &fval1, &fval2);
		}
		else
		{
			sscanf(servbuf, "%s %d %d\n", type_buf, &val1, &val2);
		}
		
		if(strcmp(type_buf, "add") == 0)
		{
			i_result = (int)(val1 + val2);
		}
		if(strcmp(type_buf, "div") == 0)
		{
			i_result = (int)(val1 / val2);
		}
		if(strcmp(type_buf, "mul") == 0)
		{
			i_result = (int)(val1 * val2);
		}
		if(strcmp(type_buf, "sub") == 0)
		{
			i_result = (int)(val1 - val2);
		}
		
		if(strcmp(type_buf, "fadd") == 0)
		{
			d_result = fval1 + fval2;
		}
		if(strcmp(type_buf, "fdiv") == 0)
		{
			d_result = (fval1 / fval2);
		}
		if(strcmp(type_buf, "fmul") == 0)
		{
			d_result = fval1 * fval2;
		}
		if(strcmp(type_buf, "fsub") == 0)
		{
			d_result = fval1 - fval2;
		}

		if(strchr(type_buf, 'f') != NULL)
		{
			sprintf(res, "%8.8g\n", d_result);
		}
		else
		{
			sprintf(res, "%d\n", i_result);
		}

		// Sends over calculation
		write(sockfd, res, strlen(res));

		memset(&servbuf, 0, sizeof(servbuf));
		read(sockfd, &servbuf, sizeof(servbuf));
		printf("%s My answer = %s", servbuf, res);
	}
	else
	{
		// Sends off error to Server and Displays where it went wrong for client
		char clibuf[MAX] = "Error";
		write(sockfd, clibuf, strlen(clibuf));
		printf("SERVER ERROR:%s\n", servbuf);

	}
  
  close(sockfd);
  return 0;

}
