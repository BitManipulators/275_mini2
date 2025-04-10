#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <vector>


#include "yaml_parser.hpp"

void Config::print_config () {
    
    std::cout << "Total Partitions: " << total_partitions << std::endl;
    std::cout << "Deployment Name: " << name << std::endl;
    std::cout << "Deployment Version: " << version << std::endl;
    std::cout << "Deployment Description: " << description << std::endl;

    for (const auto& process : processes) {
        std::cout << "Process Rank: " << process.second.rank << ", Port: " << process.second.port << std::endl;
        std::cout << "  Logical Neighbors: " << std::endl;
        for (const auto& neighbor : process.second.logical_neighbors) {
            std::cout << "    IP: " << neighbor.ip << ", Port: " << neighbor.port << std::endl;
        }
    }
}

std::vector<std::string> Config::get_logical_neighbors(int rank){

    auto my_neighbors = processes[rank].logical_neighbors;
    std::vector<std::string> peer_addresses ;
    for (const auto& n : my_neighbors){

        std::string address = n.ip + ":" + std::to_string(n.port);
        peer_addresses.push_back(address);

    }

    return peer_addresses;
}

int Config::getPortNumber(int rank){

    return processes[rank].port;

}

std::string Config::getIP(int rank){

    return processes[rank].ip;

}

std::string Config::getaddress(int rank){

    return processes[rank].ip + ":" + std::to_string(processes[rank].port);

}

int Config::getTotalWorkers(){

    std::cout << "Total Workers - " << total_partitions << std::endl;
    return total_partitions;
}

