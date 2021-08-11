CC = gcc
CC_FLAGS = -w -g



all: test client server


server.o: server.cpp 
	$(CXX) -Wall -c server.cpp -I.

client.o: client.cpp 
	$(CXX) -Wall -c client.cpp -I.

main.o: main.c
	$(CXX) -Wall -I. -c main.c


test: main.o
	$(CXX) -I./ -Wall -lncurses  -o test main.o 


client: client.o
	$(CXX) -L./ -Wall -o cchat client.o -lncurses

server: server.o
	$(CXX) -L./ -Wall -o cserverd server.o -lncurses


clean:
	rm *.o test cserverd cchat
