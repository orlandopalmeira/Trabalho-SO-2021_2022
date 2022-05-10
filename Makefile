all: server client

server: bin/sdstored
client: bin/sdstore

bin/sdstored: obj/sdstored.o
	gcc -g obj/sdstored.o -o bin/sdstored

obj/sdstored.o: src/sdstored.c
	gcc -Wall -g -c src/sdstored.c -o obj/sdstored.o

bin/sdstore: obj/sdstore.o
	gcc -g obj/sdstore.o -o bin/sdstore

obj/sdstore.o: src/sdstore.c
	gcc -Wall -g -c src/sdstore.c -o obj/sdstore.o

sr: 
	./bin/sdstored etc/sdstored.conf bin/sdstore-transformations

pr:
	./bin/sdstore proc-file 2 files/file0.txt output/file_out0.txt bcompress bdecompress gcompress gdecompress

cl:
	./bin/sdstore proc-file files/file0.txt output/file_out0.txt bcompress bdecompress gcompress gdecompress

cl1:
	./bin/sdstore proc-file files/file1.txt output/file_out1.txt bcompress bdecompress

cl2:
	./bin/sdstore proc-file files/file2.txt output/file_out2.txt bcompress bdecompress

term:
	./bin/sdstore TERMINATE

status:
	./bin/sdstore status

clean:
	rm obj/*.o tmp/fifo* tmp/main_fifo bin/sdstore bin/sdstored output/*