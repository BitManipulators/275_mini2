#!/bin/bash

SERVER=$1
NUM_PROCESSES=$2

for i in $(seq 0 "$NUM_PROCESSES"); do
    RANK=$i $SERVER &
done
