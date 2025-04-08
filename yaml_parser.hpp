#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string>
#include <map>
#include <vector>
#include <yaml-cpp/yaml.h>
#include <filesystem> 

// Structure to represent the logical neighbor
struct Neighbor {
    std::string ip;
    int port;
};

// Structure to represent each process
struct Process {
    int rank;
    int port;
    std::string ip;
    std::vector<Neighbor> logical_neighbors;
};

// Structure to represent the entire configuration
class Config {
    
    public :
        
        Config(){
            
            std::filesystem::path filepath = "../config/new-config.yaml";
            
            if (!std::filesystem::exists(filepath)) {
                
                //Debug how it is being called before getInstance
                std::cerr << "Config file not found: " << filepath << std::endl;
                throw std::runtime_error("Config file not found");
            }

            //std::cout << "Called config" << std::endl;

            // Load the YAML file
            YAML::Node configNode = YAML::LoadFile(filepath);

            // Parse global section
            total_partitions = configNode["global"]["total_partitions"].as<int>();

            // Parse deployment section
            name = configNode["deployment"]["name"].as<std::string>();
            version = configNode["deployment"]["version"].as<std::string>();
            description = configNode["deployment"]["description"].as<std::string>();

            int numberofworker = 0;
            // Parse processes section
            for (const auto& processNode : configNode["processes"]) {
                
                Process process;
                process.rank = processNode.second["rank"].as<int>();
                process.port = processNode.second["port"].as<int>();
                process.ip = processNode.second["ip"].as<std::string>();

                // Parse logical neighbors
                for (const auto& neighborNode : processNode.second["logical_neighbors"]) {
                    Neighbor neighbor;
                    neighbor.ip = neighborNode["ip"].as<std::string>();
                    neighbor.port = neighborNode["port"].as<int>();
                    process.logical_neighbors.push_back(neighbor);
                }

                processes[process.rank] = process ;
                numberofworker++;
            }

            if (total_partitions != numberofworker){
                throw std::runtime_error("Incorrect Config file");
            }


        }
        
        void print_config ();
        std::vector<std::string> get_logical_neighbors (int rank);
        int getPortNumber(int rank);
        int getTotalWorkers();
        std::string getaddress(int rank);

        private :
        
                int total_partitions;
                std::string name;
                std::string version;
                std::string description;
                std::map<int, Process> processes;
};

#endif


