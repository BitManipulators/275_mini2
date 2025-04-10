#!/bin/bash

SERVER=$1
NUM_PROCESSES=$(($2 - 1))

for i in $(seq 0 "$NUM_PROCESSES"); do
    RANK=$i $SERVER 1>/dev/null &
done
