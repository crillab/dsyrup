#!/bin/sh

if [ "$2"=="8" ]; then
    ./bin/glucose-syrup -nthreads=$2 -model -maxmemory=20000 $1
else 
    if [ "$2"=="64" ]; then
        ./bin/glucose-syrup -nthreads=32 -model -maxmemory=450000 $1
    else
        ./bin/glucose-syrup -nthreads=$2 -model $1
    fi
fi

