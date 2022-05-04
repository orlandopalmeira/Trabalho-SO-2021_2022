CC = gcc
CFLAGS = -g -Wall

all: sdstore sdstored

sr: 
	./sdstored ../etc/config.txt ../bin 

cl:
	./sdstore proc-file ../files/file0.csv ../output/file_out0.csv compress decompress

term:
	./sdstore TERMINATE

status:
	./sdstore status

debs:
	gdb --args ./sdstored ../etc/config.txt ../bin 


clean:
	rm ../tmp/main_fifo ../tmp/fifo* sdstored sdstore