#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>

/* You will to add includes here */
#include<sys/socket.h>
#include<sys/types.h>
#include<string.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<netdb.h>
#include <string>
#include <iostream>
#include <vector>
#include <chrono>

// Included to get the support library Client
#include <calcLib.h>

#include "protocol.h"

using namespace std;
/* Needs to be global, to be rechable by callback and main */
int loopCount=0;
int terminate_loop=0;

// Store information
// Public for the purpose of ease of use with certain functions
vector<calcProtocol> storage;
vector<sockaddr_in> addresses;
vector<int> timers;
vector<timeval> tim;
int current_pos = 0;

int special_exception = 0;


void *get_in_addr(struct sockaddr *sa)
{
    if(sa->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

bool is_in_system(int ID, int &d)
{
  for(int i = 0; i < (int)storage.size(); i++)
  {
    if((int)htonl(storage[i].id) == ID)
    {
      d = i;
      return true;
    }
  }

  return false; // Didn't find ID
}

void decrement_timers()
{
  for(int i = 0; i < (int)timers.size(); i++)
  {
    if(timers[i] > 0)
    {
      timers[i] -= 1;
    }
  }
}

void check_timers()
{
  for(int i = 0; i < (int)timers.size(); i++)
  {
    if(timers[i] == 0)
    {
      current_pos--;
      timers[i] = -1;
    }
  }
}

void empty_storage()
{
    storage.clear();
    timers.clear();
    addresses.clear();
    tim.clear();
  printf("Storage has been ransacked.\n");
}

void create_package(calcProtocol &calc)
{
  calc.type = htons(1);
  calc.major_version = htons(1);
  calc.minor_version = htons(0);
  calc.id = htonl(randomInt());
  calc.flValue1 = 0.0;
  calc.flValue2 = 0.0;
  calc.inValue1 = htonl(0);
  calc.inValue2 = htonl(0);

  char* type_arith = randomType();
  if(strcmp("fsub", type_arith) == 0)
  {
    calc.arith = 6;
    calc.flValue1 = randomFloat();
    calc.flValue2 = randomFloat();
    calc.flResult = calc.flValue1 - calc.flValue2;
  }
  else if(strcmp("fadd", type_arith) == 0)
  {
    calc.arith = 5;
    calc.flValue1 = randomFloat();
    calc.flValue2 = randomFloat();
    calc.flResult = calc.flValue1 + calc.flValue2;
  }
  else if(strcmp("fmul", type_arith) == 0)
  {
    calc.arith = 7;
    calc.flValue1 = randomFloat();
    calc.flValue2 = randomFloat();
    calc.flResult = calc.flValue1 * calc.flValue2;
  }
  else if(strcmp("fdiv", type_arith) == 0)
  {
    calc.arith = 8;
    calc.flValue1 = randomFloat();
    calc.flValue2 = randomFloat();
    calc.flResult = calc.flValue1 / calc.flValue2;
  }
  else if(strcmp("add", type_arith) == 0)
  {
    calc.arith = 1;
    calc.inValue1 = randomInt();
    calc.inValue2 = randomInt();
    calc.inResult = htonl((calc.inValue1 + calc.inValue2));

    calc.inValue1 = htonl(calc.inValue1);
    calc.inValue2 = htonl(calc.inValue2);
  }
  else if(strcmp("sub", type_arith) == 0)
  {
    calc.arith = 2;
    calc.inValue1 = randomInt();
    calc.inValue2 = randomInt();
    calc.inResult = htonl((calc.inValue1 - calc.inValue2));

    calc.inValue1 = htonl(calc.inValue1);
    calc.inValue2 = htonl(calc.inValue2);
  }
  else if(strcmp("mul", type_arith) == 0)
  {
    calc.arith = 3;
    calc.inValue1 = randomInt();
    calc.inValue2 = randomInt();
    calc.inResult = htonl((calc.inValue1 * calc.inValue2));

    calc.inValue1 = htonl(calc.inValue1);
    calc.inValue2 = htonl(calc.inValue2);
  }
  else if(strcmp("div", type_arith) == 0)
  {
    calc.arith = 4;
    calc.inValue1 = randomInt();
    calc.inValue2 = randomInt();
    calc.inResult = htonl((calc.inValue1 / calc.inValue2));

    calc.inValue1 = htonl(calc.inValue1);
    calc.inValue2 = htonl(calc.inValue2);
  }

  calc.arith = htonl(calc.arith);
}

// Check if an address is already inside the database
bool is_in_system(sockaddr_in* check)
{
  char compare[20] = {0};
  inet_ntop(check->sin_family, check, (char*)compare, INET6_ADDRSTRLEN);
  for(int i = 0; i < (int)addresses.size(); i++)
  {
    char in_store[20] = {1}; 
    inet_ntop(addresses[i].sin_family, &addresses[i], (char*)in_store, INET_ADDRSTRLEN);
    if(strstr(compare, in_store) != NULL)
    {
      return true;
    }
  }
  // Add address to storage
  addresses.push_back(*check);
  return false; // Address was not in system
}


  /* Call back function, will be called when the SIGALRM is raised when the timer expires. */
  void checkJobbList(int signum)
  {
  // As anybody can call the handler, its good coding to check the signal number that called it.
  if(signum == SIGALRM)
  {
    struct timeval comp;
    gettimeofday(&comp, NULL);
    int first = comp.tv_sec;
    for(int i = 0; i < (int)timers.size(); i++)
    {
      int second = timers[i];
      if(first - second >= 10)
      {
        if(timers[i] != -1)
          current_pos--;
          
        timers[i] = -1;
      }
    }
  }
    return;
  }


int main(int argc, char *argv[]){
  
  if(argc != 2)
  {
    printf("No more/less than 1 argument needs to be present. Retry.\n");
    exit(0);
  }

  initCalcLib();
  /* Do more magic */
  calcProtocol servBuf;
  sockaddr_in servaddr;
  memset(&servaddr, 0, sizeof(sockaddr_in));
  struct addrinfo hints, *servinfo, *p;

  // Divide string into two parts 
  char* adress = strtok(argv[1], ":");
  char* port = strtok(NULL, "");
  printf("Host %s ", adress);
  printf("and port %s\n", port);

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_DGRAM;
  
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

  
  socklen_t len = sizeof servaddr;
  /* 
     Prepare to setup a reoccurring event every 10s. If it_interval, or it_value is omitted, it will be a single alarm 10s after it has been set. 
  */
  struct itimerval alarmTime;
  alarmTime.it_interval.tv_sec=10;
  alarmTime.it_interval.tv_usec=10;
  alarmTime.it_value.tv_sec=10;
  alarmTime.it_value.tv_usec=10;

  /* Regiter a callback function, associated with the SIGALRM signal, which will be raised when the alarm goes of */
  signal(SIGALRM, checkJobbList);
  setitimer(ITIMER_REAL,&alarmTime,NULL); // Start/register the alarm. 
  
  while(1)
  {
    
    // Check if anything new has arrived
    int n = recvfrom(sockfd, &servBuf, sizeof(servBuf), 0, (struct sockaddr*)&servaddr, &len);

    if(n == sizeof(calcMessage))
    {
      // Type-cast calcProtocl to calcMessage
      calcMessage* message = (calcMessage*)&servBuf;
      int type = ntohs(message->type);
      int protocol = ntohs(message->protocol);
      int major = ntohs(message->major_version);
      int minor = ntohs(message->minor_version);

      if(type == 22 && protocol == 17 && major == 1 && minor == 0)
      {
        // Check if sender is already in system
        if(!is_in_system(&servaddr))
        {
          printf("New Client.\n");
          // Create a new package
          calcProtocol newPack;
          create_package(newPack);
          int send = sendto(sockfd, &newPack, sizeof(newPack),
            0, (struct sockaddr *)&servaddr, sizeof(servaddr));

          if(send == -1)
          {
            perror("Error : Send package");
          }
          else
          {
            current_pos++;
          }
          struct timeval current_time;
          gettimeofday(&current_time, NULL);
          timers.push_back(current_time.tv_sec);
          storage.push_back(newPack);
        }
        else
        {
          printf("Already in system. Ignoring request.\n");
        }
      }
      else
      {
        printf("Wrong Protocol.\n");
        calcMessage error;
        error.type = htons(2);
        error.message = htonl(2);
        error.protocol = htons(17);
        error.major_version = htons(1);
        error.minor_version = htons(0);

        int send = sendto(sockfd, &error, sizeof(error),
            0, (struct sockaddr *)&servaddr, sizeof(servaddr));

        if (send == -1)
        {
          perror("Error : Send wrong protocol");
        }
      }
    }
    else if(n == sizeof(calcProtocol))
    {
      // Pack up reponse
      calcProtocol newProt;
      newProt.type = ntohs(servBuf.type);
      newProt.major_version = ntohs(servBuf.major_version);
      newProt.minor_version = ntohs(servBuf.minor_version);
      newProt.id = ntohl(servBuf.id);
      newProt.arith = ntohl(servBuf.arith);
      newProt.inValue1 = ntohl(servBuf.inValue1);
      newProt.inValue2 = ntohl(servBuf.inValue2);
      newProt.inResult = ntohl(servBuf.inResult);
      newProt.flValue1 = servBuf.flValue1;
      newProt.flValue2 = servBuf.flValue2;
      newProt.flResult = servBuf.flResult;

      int spot = 0;
      int ip = 0;
      if(is_in_system(newProt.id, spot))
      {
        special_exception++;
        ip = spot;

        if(timers[spot] > 0)
        {
          // Is validated, check response, send back result
          calcMessage resp;
          resp.message = htonl(2); // Pessimistic start, we assume they respond incorrectly.
          resp.type = htons(2);
          resp.protocol = htons(17);
          resp.major_version = htons(1);
          resp.minor_version = htons(0);
        
          if(newProt.arith >= 5)
          {
            if(newProt.flResult == storage[spot].flResult)
            {
              printf("Value 1: %8.8g\n", storage[spot].flValue1);
              printf("Value 2: %8.8g\n", storage[spot].flValue2);
              printf("Result: %8.8g\n", storage[spot].flResult);
              resp.message = htonl(1);
              printf("Float response was correct. %d\n", spot);
            }
          }
          else
          {
            if(newProt.inResult == (int)ntohl(storage[spot].inResult))
            {
              resp.message = htonl(1);
              printf("Value 1: %d\n", (int)ntohl(storage[spot].inValue1));
              printf("Value 2: %d\n", (int)ntohl(storage[spot].inValue2));
              printf("Result: %d\n", (int)ntohl(storage[spot].inResult));
              printf("Integer response was correct. %d\n", spot);
            }
          }
          int send = sendto(sockfd, &resp, sizeof(resp),
            0, (struct sockaddr *)&addresses[ip], sizeof(addresses[ip]));

          if(send == -1)
          {
            perror("Error : Send confirmation");
          }
          else
          {
            current_pos--;
          }

          if(current_pos <= 0)
          {
            // This is to empty storage however might lead to unanswered calls from clients.
            //empty_storage();
          }
          continue;
        }
        else if(special_exception > 0) // Special exception for some clients.
        {
          ip = special_exception;
          spot = special_exception;
          if(timers[special_exception] > 0)
          {
            // Is validated, check response, send back result
          calcMessage resp;
          resp.message = htonl(2); // Pessimistic start, we assume they respond incorrectly.
          resp.type = htons(2);
          resp.protocol = htons(17);
          resp.major_version = htons(1);
          resp.minor_version = htons(0);
        
          if(newProt.arith >= 5)
          {
            if(newProt.flResult == storage[spot].flResult)
            {
              printf("Value 1: %8.8g\n", storage[spot].flValue1);
              printf("Value 2: %8.8g\n", storage[spot].flValue2);
              printf("Result: %8.8g\n", storage[spot].flResult);
              resp.message = htonl(1);
              printf("Float response was correct. %d\n", spot);
            }
          }
          else
          {
            if(newProt.inResult == (int)ntohl(storage[spot].inResult))
            {
              resp.message = htonl(1);
              printf("Value 1: %d\n", (int)ntohl(storage[spot].inValue1));
              printf("Value 2: %d\n", (int)ntohl(storage[spot].inValue2));
              printf("Result: %d\n", (int)ntohl(storage[spot].inResult));
              printf("Integer response was correct. %d\n", spot);
            }
          }
          int send = sendto(sockfd, &resp, sizeof(resp),
            0, (struct sockaddr *)&addresses[ip], sizeof(addresses[ip]));

          if(send == -1)
          {
            perror("Error : Send confirmation");
          }
          else
          {
            current_pos--;
          }

          if(current_pos <= 0)
          {
            // This is to empty storage however might lead to unanswered calls from clients.
            //empty_storage();
          }

          continue;
        }
        else
        {
          // Out of time to respond.
          printf("Slow client tried to answer, ignoring request. %d\n", special_exception);
          calcMessage error;
          error.type = htons(2);
          error.message = htonl(2);
          error.protocol = htons(17);
          error.major_version = htons(1);
          error.minor_version = htons(0);

          sendto(sockfd, &error, sizeof(error),
            0, (const struct sockaddr *)&addresses[ip], sizeof(addresses[ip]));
        }
      }
        
      }
      else
      {
        printf("No match on ID, ignoring request.\n");
        calcMessage error;
        error.type = htons(2);
        error.message = htonl(2);
        error.protocol = htons(17);
        error.major_version = htons(1);
        error.minor_version = htons(0);

        int send = sendto(sockfd, &error, sizeof(error),
          0, (const struct sockaddr *)&servaddr, sizeof(servaddr));

        if(send == -1)
        {
          perror("Error : No Matching ID");
        }
      }
    }
    loopCount++;
  }

  return(0);


  
}
