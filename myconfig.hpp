#ifndef MYCONFIG_HPP
#define MYCONFIG_HPP

#include <mutex>
#include <string>
#include <memory>
#include <vector>
#include <filesystem>

#include "yaml_parser.hpp"

class MyConfig {

    public :
        static MyConfig* getInstance();
        int getRank();
        std::vector<std::string> getLogicalNeighbors();
        int getTotalNumberofProcess();
        int getPortNumber();
        std::string getIP();
        bool isSameNodeProcess(int target_rank);
        

    private :
        
        int rank;
        static std::mutex mtx;
        static MyConfig* instance;
        Config config;
        MyConfig (int n);
        

};

#endif