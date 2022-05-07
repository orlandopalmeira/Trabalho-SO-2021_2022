CC = gcc
CFLAGS = -g -Wall

all: sdstore sdstored

sr: 
	./sdstored ../etc/config.txt ../bin

pr:
	./sdstore proc-file 2 ../files/file0.txt ../output/file_out0.txt bcompress bdecompress gcompress gdecompress

cl:
	./sdstore proc-file ../files/file0.txt ../output/file_out0.txt bcompress bdecompress gcompress gdecompress

cl1:
	./sdstore proc-file ../files/file1.txt ../output/file_out1.txt bcompress bdecompress

cl2:
	./sdstore proc-file ../files/file2.txt ../output/file_out2.txt bcompress bdecompress

term:
	./sdstore TERMINATE

status:
	./sdstore status

debs:
	gdb --args ./sdstored ../etc/config.txt ../bin 


clean:
	rm ../tmp/main_fifo ../tmp/fifo* sdstored sdstore ../output/*