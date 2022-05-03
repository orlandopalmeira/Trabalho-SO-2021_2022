CC = gcc
CFLAGS = -g -Wall

all: sdstore sdstored

sr: 
	./sdstored ../etc/config.txt ../bin 

cl:
	./sdstore proc-file ent saida compress decompress

term:
	./sdstore TERMINATE


clean:
	rm ../tmp/main_fifo ../tmp/fifo* sdstored sdstore