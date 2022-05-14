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

pr1:
	./bin/sdstore proc-file -p 1 files/file0.txt output/file_out0.txt nop bcompress encrypt

pr2:
	./bin/sdstore proc-file 2 files/file0.txt output/file_out0.txt bcompress

pr3:
	./bin/sdstore proc-file 3 files/file0.txt output/file_out0.txt gcompress gcompress gcompress gcompress

pr4:
	./bin/sdstore proc-file 4 files/file0.txt output/file_out0.txt bdecompress

pr5:
	./bin/sdstore proc-file 5 files/file0.txt output/file_out0.txt gcompress bdecompress

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
	rm obj/*.o tmp/p* tmp/main_fifo bin/sdstore bin/sdstored output/*