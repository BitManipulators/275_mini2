#ifndef MYCONFIG_HPP
#define MYCONFIG_HPP

#include <mutex>
#include <string>
#include <memory>
#include <vector>
#include "yaml_parser.hpp"



class MyConfig {

    public :
        static MyConfig* getInstance();
        int getRank();
        std::vector<std::string> getLogicalNeighbors();
        int getTotalNumberofProcess();
        int getPortNumber();

    private :
        
        int rank;
        static std::mutex mtx;
        static MyConfig* instance;
        Config config;
        
        MyConfig(int n): config("/home/suriya-018231499/sem2/cmpe275/mini2/main/275_mini2/new-config.yaml"){
            rank = n;
            
        }

};

#endif