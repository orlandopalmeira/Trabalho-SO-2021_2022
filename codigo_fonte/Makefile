CC = gcc
CFLAGS = -g -Wall

all: client server

client:
	gcc -g -Wall sdstore.c -o client

server:
	gcc -g -Wall sdstored.c -o server


sr: 
	./server ../etc/config.txt ../bin 


clean:
	rm ../tmp/main_fifo ../tmp/fifo* server client