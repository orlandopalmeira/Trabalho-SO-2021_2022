#!/bin/bash
#Teste de pedidos impossíveis

./bin/sdstore proc-file files/file0.txt output/result_imp1.txt bcompress bcompress bcompress bcompress bcompress bcompress bcompress bcompress bcompress bcompress bcompress bcompress bcompress bcompress bcompress bcompress &
./bin/sdstore proc-file files/file0.txt output/result_imp2.txt bdecompress bdecompress bdecompress bdecompress bdecompress bdecompress bdecompress bdecompress bdecompress bdecompress bdecompress bdecompress bdecompress bdecompress bdecompress bdecompress &
./bin/sdstore proc-file files/file0.txt output/result_imp3.txt gcompress gcompress gcompress gcompress gcompress gcompress gcompress gcompress gcompress gcompress gcompress gcompress gcompress gcompress gcompress gcompress &
./bin/sdstore proc-file files/file0.txt output/result_imp4.txt gdecompress gdecompress gdecompress gdecompress gdecompress gdecompress gdecompress gdecompress gdecompress gdecompress gdecompress gdecompress gdecompress gdecompress gdecompress gdecompress &
./bin/sdstore proc-file files/file0.txt output/result_imp5.txt nop nop nop nop nop nop nop nop nop nop nop nop nop nop nop nop &
./bin/sdstore proc-file files/file0.txt output/result_imp6.txt encrypt encrypt encrypt encrypt encrypt encrypt encrypt encrypt encrypt encrypt encrypt encrypt encrypt encrypt encrypt encrypt &
./bin/sdstore proc-file files/file0.txt output/result_imp7.txt decrypt decrypt decrypt decrypt decrypt decrypt decrypt decrypt decrypt decrypt decrypt decrypt decrypt decrypt decrypt decrypt &