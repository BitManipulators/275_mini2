#include <iostream>

#include "myconfig.hpp"



MyConfig* MyConfig::instance = nullptr;
std::mutex MyConfig::mtx;


//std::filesystem::path MyConfig::path = "../config/new-config.yaml";  

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

MyConfig::MyConfig(int n){
    rank = n;
            
}

int MyConfig::getRank(){
    return rank;
}

std::vector<std::string> MyConfig::getLogicalNeighbors(){
    return config.get_logical_neighbors(rank);

}

bool MyConfig::isSameNodeProcess(int target_rank){

    std::string my_ip = config.getIP(rank); 
    std::string target_ip = config.getIP(target_rank);
    return  my_ip ==  target_ip;



}

int MyConfig::getTotalNumberofProcess(){
    return config.getTotalWorkers();
}

int MyConfig::getPortNumber(){
    return config.getPortNumber(rank);
}

std::string MyConfig::getIP(){
    return config.getIP(rank);
}


