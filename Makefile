CC = gcc
CC_FLAGS = -w -g



all: client server


server.o: server.cpp 
	$(CXX) -Wall -c server.cpp -I.

client.o: client.cpp 
	$(CXX) -Wall -c client.cpp -I.


client: client.o
	$(CXX) -L./ -Wall -o client client.o -lncurses

server: server.o
	$(CXX) -L./ -Wall -o server server.o -lncurses


clean:
	rm *.o server client
