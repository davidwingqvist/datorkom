#include <stdio.h>
#include <stdlib.h>
/* You will to add includes here */
#include<sys/socket.h>
#include<sys/types.h>
#include<string.h>
#include<netinet/in.h>
#include<unistd.h>
#include<arpa/inet.h>
#include <errno.h>
#include<netdb.h>

// Included to get the support library
#include <calcLib.h>

#include "protocol.h"
int terminate = 0; // How many times we will redo loop until someone responds
int loopCount = 0; // Counts in seconds

// Does calculation on integers and returns an integer
int integer_calculate(int first, int second, int calcType)
{
  int result;
  if(calcType == 1) // add
  {
    result = (int)first + (int)second;
  }
  else if(calcType == 2) // sub
  {
    result = (int)first - (int)second;
  }
  else if(calcType == 3) // mul
  {
    result = (int)(first * second);
  }
  else if(calcType == 4) // div
  {
    result = (int)(first / second);
  }
    // Do calculations then return it as result
  printf("(client)integer result: %d\n", result); // TO BE REMOVED
  return result;
}

// Does calculation on double and returns a double
double float_calculate(double first, double second, int calcType)
{
  double result;
    if(calcType == 5) // fadd
    {
      result = first + second;
    }
    else if(calcType == 6) // fsub
    {
      result = first - second;
    }
    else if(calcType == 7) // fmul
    {
      result = first * second;
    }
    else if(calcType == 8) // fdiv
    {
      result = first / second;
    }

    printf("(client)float result: %lf\n", result); // TO BE REMOVED
    return result;
}

int main(int argc, char *argv[])
{
  if(argc != 2)
  {
    printf("No more/less than 1 argument needs to be present. Retry.\n");
    exit(0);
  }
  
  calcMessage cliBuf;
  calcProtocol cliResp; // Client Response

  calcProtocol servBuf;

  // Translated package details
  int type = 0;
  int minor_version = 0;
  int major_version = 0;
  int id = 0;
  int arith = 0;
  int inVal1 = 0;
  int inVal2 = 0;
  int inRes = 0;
  double val1 = 0, val2 = 0, fresult = 0;
  int result = 0;

  socklen_t servLen = 0;
  bool packRecv = false; // Confirmation that package recieved
  bool packSent = false;

  // Set up our package to be sen
  cliBuf.type = htons(22); // client to server - 22
  cliBuf.message = htonl(0);
  cliBuf.protocol = htons(17); // UDP - 17
  cliBuf.major_version = htons(1);
  cliBuf.minor_version = htons(0);

 struct addrinfo hints, *servinfo, *p;

  // Divide string into two parts 
  char* adress = strtok(argv[1], ":");
  char* port = strtok(NULL, "");
  printf("Host %s ", adress);
  printf("and port %s\n", port);

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_flags = AI_CANONNAME;

  struct timeval tv;
  tv.tv_sec = 3;
  tv.tv_usec = 0;
  
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
      break;
  }

  freeaddrinfo(servinfo);
  setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

  // Send over package to server
  int len = sendto(sockfd, &cliBuf, sizeof(cliBuf),
            0, p->ai_addr, p->ai_addrlen);

  if(len == -1)
  {
    perror("Package failed");
  }


  while(terminate < 3)
  {
      int n = recvfrom(sockfd, &servBuf, sizeof(servBuf), 0, p->ai_addr, &p->ai_addrlen);

      if(n == -1)
      {
        if(errno == EAGAIN)
        {
            terminate++;
            printf("TIMEOUT!\n");
            continue;
        }
      }
      else if(n >= (int)sizeof(calcProtocol))
      {
        type = ntohs(servBuf.type);
        minor_version = ntohs(servBuf.minor_version);
        major_version = ntohs(servBuf.major_version);
        id = ntohl(servBuf.id);
        arith = ntohl(servBuf.arith);
        inVal1 = ntohl(servBuf.inValue1);
        inVal2 = ntohl(servBuf.inValue2);
        val1 = servBuf.flValue1;
        val2 = servBuf.flValue2;
        packRecv = true; // confirmation package was recieved

        // Load in the float values
        if(arith >= 5)
        {
          printf("(Server)First: %lf, Second: %lf\n", val1, val2);
          fresult = float_calculate(val1, val2, arith); // Calculate the result of arith sent by server
        }
        else // else load in the integer values
        {
          printf("(Server)First: %d, Second: %d\n", inVal1, inVal2);
          result = integer_calculate(inVal1, inVal2, arith);
        }

        // Set up calcProtocol to resend result
        cliResp.type = htons(22);
        cliResp.minor_version = htons(0);
        cliResp.major_version = htons(1);
        cliResp.id = htonl(id);
        cliResp.arith = htonl(arith);
        cliResp.inValue1 = htonl(inVal1);
        cliResp.inValue2 = htonl(inVal2);
        cliResp.inResult = htonl(result);
        cliResp.flValue1 = val1;
        cliResp.flValue2 = val2;
        cliResp.flResult = fresult;

        len = sendto(sockfd, &cliResp, sizeof(cliResp),
            0, p->ai_addr, p->ai_addrlen);
        packSent = true;
      }
      else
      {
        if(!packRecv)
        {
          // Displays error message to user.
          calcMessage* error = (calcMessage*)&servBuf;
          int error_type = ntohs(error->type);
          int error_message = ntohl(error->message);
          int error_major = ntohs(error->major_version);
          int error_minor = ntohs(error->minor_version);
          if(error_type == 2 && error_message == 2 && error_major == 1 &&
          error_minor == 0)
          {
            printf("(Server)NOT OK!\n");
            close(sockfd);
            exit(0);
          }
        }
        if(packSent)
        {
          calcMessage* response = (calcMessage*)&servBuf;
          calcProtocol* response2 = (calcProtocol*)&servBuf;
          int resp_message = ntohl(response->message);
          // Correct answer
          if(resp_message == 1)
          {
            printf("(server)Value 1: %d\n", htonl(response2->inValue1));
            printf("(server)Value 2: %d\n", htonl(response2->inValue2));

            printf("(server)Float Value 1: %8.8g\n", response2->flValue1);
            printf("(server)Float Value 2: %8.8g\n", response2->flValue2);
            printf("(Server)OK\n");
            close(sockfd);
            exit(0);
          }
          else if(resp_message == 2) // Incorrect answer
          {
            printf("(Server)NOT OK\n");
            close(sockfd);
            exit(0);
          }
        }
      }
      
      // Make sure we send resend it 2 times which adds up to three with initial transmission.
      if(!packRecv && terminate < 3)
      {
        printf("Resending package...\n");
        // Resend package
        len = sendto(sockfd, &cliBuf, sizeof(cliBuf),
              0, p->ai_addr, p->ai_addrlen);
      }
  }
  if(!packRecv || !packSent)
  {
    printf("Server timed out.\n");
  }
  else
    printf("End of session.\n");
  close(sockfd);
  exit(0);
}
