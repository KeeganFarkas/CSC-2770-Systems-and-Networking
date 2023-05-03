PWD = $(shell pwd)

all: server

server: server.cpp
	g++ -c -o server.o server.cpp
	g++ -g -Wall -pedantic server.cpp libastyle-2.06d.so -o server

run: all
	LD_LIBRARY_PATH=$(PWD): ./server

clean:
	rm *.o server