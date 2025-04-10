#!/bin/bash

pkill -f 'sync.*server' --signal SIGINT
sleep 1
pkill -f 'sync.*server' --signal SIGTERM
#sleep 1
#pkill -f 'sync.*server' --signal SIGABRT
