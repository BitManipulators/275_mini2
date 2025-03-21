#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <vector>
#include <yaml-cpp/yaml.h>

#include "yaml_parser.hpp"

DeploymentConfig parseConfig(const std::string& configFile) {
    DeploymentConfig config;

    try {
        YAML::Node rootNode = YAML::LoadFile(configFile);

        if (rootNode["deployment"]) {
            auto deployment = rootNode["deployment"];
            config.name = deployment["name"].as<std::string>();
            config.version = deployment["version"].as<std::string>();
            config.description = deployment["description"].as<std::string>();
        }

        if (rootNode["machines"]) {
            auto machines = rootNode["machines"];
            for (YAML::const_iterator it = machines.begin(); it != machines.end(); ++it)
            {
                std::string id = it->first.as<std::string>();
                YAML::Node machineNode = it->second;

                MachineConfig machine;
                machine.hostname = machineNode["hostname"].as<std::string>();
                machine.ip = machineNode["ip"].as<std::string>();

                auto resources = machineNode["resources"];
                machine.resources.cpu = resources["cpu"].as<int>();
                machine.resources.memory = resources["memory"].as<std::string>();
                machine.resources.disk = resources["disk"].as<std::string>();

                config.machines[id] = machine;
            }
        }

        if (rootNode["processes"]) {
            auto processes = rootNode["processes"];
            for (YAML::const_iterator it = processes.begin(); it != processes.end(); ++it)
            {
                std::string id = it->first.as<std::string>();
                YAML::Node processNode = it->second;

                ProcessConfig process;
                process.name = processNode["name"].as<std::string>();
                process.machine = processNode["machine"].as<std::string>();
                process.executable = processNode["executable"].as<std::string>();
                process.port = processNode["port"].as<int>();

                if (processNode["environment"]) {
                    auto env = processNode["environment"];
                    for (YAML::const_iterator it = env.begin(); it != env.end(); ++it)
                    {
                        std::string key = it->first.as<std::string>();
                        std::string value = it->second.as<std::string>();
                        process.environment.variables[key] = value;
                    }
                }

                if (processNode["depends_on"]) {
                    auto deps = processNode["depends_on"];
                    for (const auto& dep : deps) {
                        process.depends_on.push_back(dep.as<std::string>());
                    }
                }

                config.processes[id] = process;
            }
        }

        if (rootNode["overlay"] && rootNode["overlay"]["connections"]) {
            auto connections = rootNode["overlay"]["connections"];
            for (const auto& connectionNode : connections) {
                ConnectionConfig conn;
                conn.from = connectionNode["from"].as<std::string>();
                conn.to = connectionNode["to"].as<std::string>();
                conn.type = connectionNode["type"].as<std::string>();

                if (connectionNode["config"]) {
                    auto connConfig = connectionNode["config"];
                    conn.config["max_message_size"] = connConfig["max_message_size"].as<int>();
                    conn.config["keep_alive_timeout"] = connConfig["keep_alive_timeout"].as<int>();
                }

                config.overlay.connections.push_back(conn);
            }
        }

        if (rootNode["deployment_settings"]) {
            auto settings = rootNode["deployment_settings"];

            if (settings["start_order"]) {
                auto order = settings["start_order"];
                for (const auto& processName : order) {
                    config.deployment_settings.start_order.push_back(processName.as<std::string>());
                }
            }

            if (settings["health_checks"]) {
                auto health = settings["health_checks"];
                config.deployment_settings.health_checks["interval"] = health["interval"].as<int>();
                config.deployment_settings.health_checks["timeout"] = health["timeout"].as<int>();
                config.deployment_settings.health_checks["retries"] = health["retries"].as<int>();
            }

            if (settings["logging"]) {
                auto logging = settings["logging"];
                config.deployment_settings.logging["level"] = logging["level"].as<std::string>();
                config.deployment_settings.logging["path"] = logging["path"].as<std::string>();

                if (logging["rotate"]) {
                    auto rotate = logging["rotate"];
                    for(YAML::const_iterator it = rotate.begin(); it != rotate.end(); ++it)
                    {
                        std::string key = "rotate_" + it->first.as<std::string>();
                        config.deployment_settings.logging[key] = it->second.as<std::string>();
                    }
                }
            }


            if (settings["restart_policy"]) {
                auto restart = settings["restart_policy"];
                config.deployment_settings.restart_policy["condition"] = restart["condition"].as<std::string>();
                config.deployment_settings.restart_policy["max_attempts"] = std::to_string(restart["max_attempts"].as<int>());
                config.deployment_settings.restart_policy["window"] = std::to_string(restart["window"].as<int>());
            }
        }

    } catch (const YAML::Exception& e) {
        std::cerr << "Error parsing YAML: " << e.what() << std::endl;
    }

    return config;
}

void displayConfig(const DeploymentConfig& config) {
    std::cout << "Deployment: " << config.name << " (v" << config.version << ")" << std::endl;
    std::cout << "Description: " << config.description << std::endl;

    std::cout << "\nMachines:" << std::endl;
    for (const auto& [machineId, machine] : config.machines) {
        std::cout << "  - " << machineId << ": " << machine.hostname
                  << " (" << machine.ip << ")" << std::endl;
        std::cout << "    Resources: " << machine.resources.cpu << " CPUs, "
                  << machine.resources.memory << " RAM, "
                  << machine.resources.disk << " Disk" << std::endl;
    }

    std::cout << "\nProcesses:" << std::endl;
    for (const auto& [processId, process] : config.processes) {
        std::cout << "  - " << processId << ": " << process.name << std::endl;
        std::cout << "    Machine: " << process.machine << std::endl;
        std::cout << "    Executable: " << process.executable << std::endl;
        std::cout << "    Port: " << process.port << std::endl;

        if (!process.depends_on.empty()) {
            std::cout << "    Dependencies: ";
            for (const auto& dep : process.depends_on) {
                std::cout << dep << " ";
            }
            std::cout << std::endl;
        }
    }

    std::cout << "\nConnections:" << std::endl;
    for (const auto& conn : config.overlay.connections) {
        std::cout << "  - " << conn.from << " -> " << conn.to << " (" << conn.type << ")" << std::endl;
        std::cout << "    Max Message Size: " << conn.config.at("max_message_size") << " bytes" << std::endl;
    }

    std::cout << "\nStart Order:" << std::endl;
    for (const auto& process : config.deployment_settings.start_order) {
        std::cout << "  - " << process << std::endl;
    }
}

std::vector<std::string> getPeerAddresses(const DeploymentConfig& config, const std::string& selfProcessId) {
    std::vector<std::string> peerAddresses;
    for (const auto &process : config.processes) {
        const std::string &processId = process.first;
        if (processId == selfProcessId) continue;
        const ProcessConfig &procConfig = process.second;
        auto machIt = config.machines.find(procConfig.machine);
        if(machIt != config.machines.end()) {
            std::string address = machIt->second.ip + ":" + std::to_string(procConfig.port);
            peerAddresses.push_back(address);
        } else {
            std::cerr << "Machine " << procConfig.machine << " not found in config." << std::endl;
        }
    }
    return peerAddresses;
}
