CC = gcc
CFLAGS = -g -Wall

all: sdstore sdstored

sr: 
	./sdstored ../etc/config.txt ../bin

cl:
	./sdstore proc-file ../files/file0.txt ../output/file_out0.txt bcompress bdecompress

term:
	./sdstore TERMINATE

status:
	./sdstore status

debs:
	gdb --args ./sdstored ../etc/config.txt ../bin 


clean:
	rm ../tmp/main_fifo ../tmp/fifo* sdstored sdstore ../output/*