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
#include <curses.h>
#include <unistd.h>
#include <vector>
#include <iostream>
#include <sstream>
#include <string>
#include <regex.h>

/*
  The reason why max is 257 is to account for the newline \n and the end of string 0\ character which gets included.
  This limits the message to 255 characters for the user which is what the assignment says!
*/
#define MAX 258
#define USERLEN 13
#define STDIN 0

struct package_data
{
  char message_owner[USERLEN];
  char message[MAX];
  char message_type[5];
};

fd_set current_sockets, ready_sockets;
int handle_connection(char* data);

int main(int argc, char *argv[]){
  
  	/* Do magic */
	if(argc != 3)
  {
    printf("ONLY 3 ARGUMENTS ARE ACCEPTED\n PROGRAM IP:PORT USERNAME\n");
    exit(0);
  }

  /*
  initscr();
  cbreak();
  echo();
  getch();
  nocbreak();
  delwin(stdscr);
  endwin();
  */

  char user_name[USERLEN];
  memset(user_name, 0, sizeof(user_name));
  strcpy(user_name, argv[2]);

  struct addrinfo hints, *servinfo, *p;
  char* address = strtok(argv[1], ":");
  char* port = strtok(NULL, "");
  
  char *expression="^[A-Za-z0-9_]+$";
  regex_t regularexpression;
  int reti;
  
  reti=regcomp(&regularexpression, expression, REG_EXTENDED);
  if(reti){
    fprintf(stderr, "Could not compile regex.\n");
    exit(1);
  }

  printf("Host %s ", address);
  printf("and port %s\n", port);
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_CANONNAME;

  int matches = 0;
  regmatch_t items;

  for(int i=2;i<argc;i++){
    if(strlen(argv[i])<=12){
      reti=regexec(&regularexpression, argv[i],matches,&items,0);
      if(!reti){
    	  printf("Nickname is accepted on client side.\n");
      } else {
	      printf("Nickname is NOT accepted on client side.\n");
        exit(EXIT_FAILURE);
      }
    } else {
        printf("Nickname is TOO LONG on client side.\n");
        exit(EXIT_FAILURE);
    }
  }


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
        close(sockfd);
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

  bool accepted = false;
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
          if(i == 0 && accepted)
          {
            char message[MAX];
            memset(message, 0 , sizeof message); 
            fgets(message, MAX, stdin);

            // +5 is to account for any extra characters that gets included automatically.
            // Mainly the MSG and space character at the sprintf below.
            char mess[MAX + 5];
            memset(mess, 0, sizeof mess);

            sprintf(mess, "MSG %s", message);
            //std::string hack(mess);

            if(strlen(message) <= 256)
            {
            
            int wr = write(sockfd, mess, strlen(mess));
            if(wr == -1)
            {
              perror("Error write : ");
              close(sockfd);
              exit(EXIT_FAILURE);
            }
            }
            else
            {
              std::cout << "Message was too big > 255. Not sent.\n";
            }
          }
          // Read for messages.
          else if(i == sockfd)
          {
            char data[1024];
            memset(data, 0, sizeof data);
            int rd = read(sockfd, data, sizeof(data));
            if(rd > 0)
            {
              char command[50];
              char nick[14];
              char message[1024];
              char buffer[1024];
              std::string stringData(data);
              //std::cout << stringData;
              while (rd > 0)
              {
                memset(command, 0, 50);
                memset(nick, 0, 14);
                memset(message, 0, 1024);
                memset(buffer, 0, 1024);

                int s = sscanf(stringData.c_str(), "%s %s %[^\n]", command, nick, message);
                //std::cout << "Command: " << command << "\n" << "Nick: " << nick << "\n" << "Message: " << message << "\n";

                if (strcmp(command, "MSG") == 0)
                {
                  if(strlen(message) == 0 || strcmp(nick, "MSG") == 0)
                  {
                    // Faulty message was sent to client, break loop here.
                    break;
                  }

                  // Display to client only if nickname isnt the same as local and message actually is something.
                  if(strcmp(nick, user_name) != 0 && strlen(message) > 0)
                    printf("%s: %s\n", nick, message);
                }
                else if(strcmp(command, "HELLO") == 0 && strcmp(nick, "1") == 0)
                {
                  memset(buffer, 0, sizeof(buffer));
                  strcpy(buffer, "NICK ");
                  strcat(buffer, user_name);
                  strcat(buffer, "\n");
                  int w = write(sockfd, buffer, strlen(buffer));
                  if(w <= 0)
                  {
                    std::cout << "Error with writing nickname\n";
                  }
                  break;
                }
                else if(strcmp(command, "OK") == 0)
                {
                  accepted = true;
                  break;
                }
                else
                {
                  // End of command list start listening for next package.
                  break;
                }

                int bytesHandled = strlen(command) + strlen(message) + strlen(nick) + 3;
                if (bytesHandled > stringData.length())
                  break;

                stringData = stringData.substr(bytesHandled, rd - bytesHandled);
                rd -= bytesHandled;
              }
            }
            else if(rd == 0)
            {
              std::cout << "Server shutdown.\n";
              regfree(&regularexpression);
              close(sockfd);
              return EXIT_SUCCESS;
            }
          }
        }
      }
      break;
    }
	  
  }

  regfree(&regularexpression);
  close(sockfd);
  return EXIT_SUCCESS;

}

int handle_connection(char* data)
{
  std::string compare(data);
  //std::cout << "String to compare: " << compare << "\n";
  if(compare.find("MSG") <= 3)
  {
    return 1;
  }

  // To handle protocol and test server
  if(compare.find("HELLO 1") != std::string::npos)
  {
    return 0;
  }
  if(compare.find("Hello 1") != std::string::npos)
  {
    return 0;
  }
  
  if(compare.find("OK") != std::string::npos)
  {
    return 3;
  }
  if(compare.find("ERROR") != std::string::npos)
  {
    return -2;
  }

  return -1;
}