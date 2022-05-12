#!/bin/bash
#teste normal
i=0

while [ $i -lt 4 ]
do
    ./bin/sdstore proc-file files/file$i.txt output/result$i.txt bcompress bcompress nop encrypt nop decrypt bdecompress bdecompress gcompress gdecompress &
    i=$((i+1))
done