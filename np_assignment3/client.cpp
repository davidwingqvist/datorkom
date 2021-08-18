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
#include <string>
#include <regex.h>

#define MAX 255
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
  strcpy(user_name, argv[2]);

  struct addrinfo hints, *servinfo, *p;
  char* address = strtok(argv[1], ":");
  char* port = strtok(NULL, "");
  
  char *expression="^[A-Za-z0-9]";
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

            char mess[MAX];
            memset(mess, 0, sizeof mess);

            strncpy(mess, "MSG ", 5);
            strncat(mess, message, MAX - 5);

            int wr = write(sockfd, mess, strlen(mess));
            if(wr == -1)
            {
              perror("Error write : ");
              close(sockfd);
              exit(EXIT_FAILURE);
            }
          }
          // Read for messages.
          else if(i == sockfd)
          {
            char message[MAX];
            memset(message, 0, sizeof message);

            int r = read(sockfd, &message, sizeof(message));
            if(r > 0)
            {
              int compare = handle_connection(message);
              //std::cout << "Result with connection: " << compare << "\n";

              if(compare == 1 && strlen(message) > 0)
              {
                std::string output = message;
                output.erase(0, 4);

                // No echoing back our message.
                output.find(user_name);
                if((output.find(user_name)) >= strlen(user_name))
                  std::cout << output;
              }
              // Allow on client side to send messages.
              else if(compare == 0)
              {
                // send over nickname.
                char name_pack[MAX];
                memset(name_pack, 0, sizeof name_pack);
                strcpy(name_pack, "NICK ");
                strcat(name_pack, user_name);

                int snd = write(sockfd, name_pack, sizeof name_pack);
                if(snd == -1)
                  perror("Error sending nickname : ");
              }
              else if(compare == -2)
              {
                std::cout << "Error, not accepted nickname\n";
                close(sockfd);
                exit(EXIT_FAILURE);
              }
              else if (compare == 3)
              {
                std::cout << "[SERVER] Connection has been accepted. Welcome to the chat room!\n";
                accepted = true;
              }
              else
              {
                std::cout << "Unkown error, client shutting down.\n";
                close(sockfd);
                exit(EXIT_FAILURE);
              }
            }
            else
            {
              // Disconnect.
              memset(message, 0, sizeof message);
              std::cout << "Server shutdown.\n";
              close(sockfd);
              exit(EXIT_SUCCESS);
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
  if(compare.find("Hello 1\n") != std::string::npos)
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