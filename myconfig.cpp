#include <iostream>

#include "myconfig.hpp"



MyConfig* MyConfig::instance = nullptr;
std::mutex MyConfig::mtx;

MyConfig* MyConfig::getInstance(){

    if (instance == nullptr){

        std::lock_guard<std::mutex> lock(mtx);
        if (instance == nullptr){
            
            const char *rankEnv = std::getenv("RANK");
            if (!rankEnv) {
                std::cerr << "RANK environment variable not set!" << std::endl;
                throw std::runtime_error("Error : RANK ENV not found");
            }
            
            int rank = std::stoi(rankEnv);
            instance = new MyConfig(rank);
        }
        
    }
    
    return instance;

}

int MyConfig::getRank(){
    return rank;
}

std::vector<std::string> MyConfig::getLogicalNeighbors(){
    return config.get_logical_neighbors(rank);

}

int MyConfig::getTotalNumberofProcess(){
    return config.getTotalWorkers();
}

int MyConfig::getPortNumber(){
    return config.getPortNumber(rank);
}


