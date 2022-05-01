CC = gcc
CFLAGS = -g -Wall

all: sdstore sdstored


server: 
	./sdstored ../etc/config.txt ../bin 

client:
	./client 0


clean:
	rm ../tmp/main_fifo ../tmp/fifo*