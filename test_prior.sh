#!/bin/bash
# teste de prioridades
i=0

while [ $i -lt 6 ]
do
    ./bin/sdstore proc-file $i files/file3.txt output/result_prior$i.txt bcompress bcompress nop bdecompress encrypt nop decrypt bdecompress &
    i=$((i+1))
done