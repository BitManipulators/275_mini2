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
    std::vector<Neighbor> logical_neighbors;
};

// Structure to represent the entire configuration
class Config {
    
    public :
        
        Config(std::filesystem::path filepath){
            
            if (!std::filesystem::exists(filepath)) {
                
                //Debug how it is being called before getInstance
                //std::cerr << "Config file not found: " << filepath << std::endl;
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

            // Parse processes section
            for (const auto& processNode : configNode["processes"]) {
                
                Process process;
                process.rank = processNode.second["rank"].as<int>();
                process.port = processNode.second["port"].as<int>();

                // Parse logical neighbors
                for (const auto& neighborNode : processNode.second["logical_neighbors"]) {
                    Neighbor neighbor;
                    neighbor.ip = neighborNode["ip"].as<std::string>();
                    neighbor.port = neighborNode["port"].as<int>();
                    process.logical_neighbors.push_back(neighbor);
                }

                processes[process.rank] = process ;
            }


        }
        
        void print_config ();
        std::vector<std::string> get_logical_neighbors (int rank);
        int getPortNumber(int rank);
        int getTotalWorkers();

        private :
        
                int total_partitions;
                std::string name;
                std::string version;
                std::string description;
                std::map<int, Process> processes;
};

#endif


