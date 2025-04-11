#!/bin/bash

echo "Asynchronous client"

echo "5 processes"
$(dirname $0)/../run.sh ./async_server 5
sleep 10

echo "1 client"
hyperfine -m 10 "/bin/bash -c './client & wait'" --warmup 10 > 1_client_10_requests_async_results.txt
hyperfine -m 25 "/bin/bash -c './client & wait'" > 1_client_25_requests_async_results.txt
hyperfine -m 50 "/bin/bash -c './client & wait'" > 1_client_50_requests_async_results.txt
hyperfine -m 100 "/bin/bash -c './client & wait'" > 1_client_100_requests_async_results.txt


echo "2 clients"
hyperfine -m 10 "/bin/bash -c './client & ./client & wait'" > 2_clients_10_requests_async_results.txt
hyperfine -m 25 "/bin/bash -c './client & ./client & wait'" > 2_clients_25_requests_async_results.txt
hyperfine -m 50 "/bin/bash -c './client & ./client & wait'" > 2_clients_50_requests_async_results.txt
hyperfine -m 100 "/bin/bash -c './client & ./client & wait'" > 2_clients_100_requests_async_results.txt


echo "4 clients"
hyperfine -m 10 "/bin/bash -c './client & ./client & ./client & ./client & wait'" > 4_clients_10_requests_async_results.txt
hyperfine -m 25 "/bin/bash -c './client & ./client & ./client & ./client & wait'" > 4_clients_25_requests_async_results.txt
hyperfine -m 50 "/bin/bash -c './client & ./client & ./client & ./client & wait'" > 4_clients_50_requests_async_results.txt
hyperfine -m 100 "/bin/bash -c './client & ./client & ./client & ./client & wait'" > 4_clients_100_requests_async_results.txt


echo "8 clients"
hyperfine -m 10 "/bin/bash -c './client & ./client & ./client & ./client & ./client & ./client & ./client & ./client & wait'" > 8_clients_10_requests_async_results.txt
hyperfine -m 25 "/bin/bash -c './client & ./client & ./client & ./client & ./client & ./client & ./client & ./client & wait'" > 8_clients_25_requests_async_results.txt
hyperfine -m 50 "/bin/bash -c './client & ./client & ./client & ./client & ./client & ./client & ./client & ./client & wait'" > 8_clients_50_requests_async_results.txt
hyperfine -m 100 "/bin/bash -c './client & ./client & ./client & ./client & ./client & ./client & ./client & ./client & wait'" > 8_clients_100_requests_async_results.txt
$(dirname $0)/../kill.sh
