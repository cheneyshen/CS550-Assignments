#!/bin/sh

file_path=/home/ubuntu/p1

for  ((n=0 ;n < 1;n++))
do
    dd if=/dev/urandom of=$file_path/test.2.$n.bin bs=1024 count=1048576
done
for  ((n=0 ;n < 900;n++))
do
    dd if=/dev/urandom of=$file_path/test.1.$n.bin bs=1024 count=1024
done
for ((n=0; n < 9000; n++))
do
    dd if=/dev/urandom of=$file_path/test.1.$n.bin bs=1024 count=1024
done
echo ===========
