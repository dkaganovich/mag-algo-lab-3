#!/usr/bin/env bash

valgrind --tool=cachegrind ./mul 1023 >>valgrind-mul-LL6.txt 2>&1
./mul-cache 1023 6144 64 12 0 >>mul-cache-lru-LL6.txt

valgrind --tool=cachegrind ./mul 1024 >>valgrind-mul-LL6.txt 2>&1
./mul-cache 1024 6144 64 12 0 >>mul-cache-lru-LL6.txt

for i in 1023 1024 1025 1040 1041 1050 1100; do
    valgrind --tool=cachegrind --LL=3145728,12,64 ./mul $i >>valgrind-mul.txt 2>&1
    valgrind --tool=cachegrind --LL=3145728,12,64 ./trans $i >>valgrind-trans.txt 2>&1
    ./mul-cache $i 3072 64 12 0 >>mul-cache-lru.txt
    ./mul-cache $i 3072 64 12 1 >>mul-cache-rr.txt
    ./trans-cache $i 3072 64 12 0 >>trans-cache-lru.txt
    ./trans-cache $i 3072 64 12 1 >>trans-cache-rr.txt
done
