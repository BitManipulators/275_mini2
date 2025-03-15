#pragma once

#include <string>
#include <map>
#include <vector>

namespace YAML {
    class Node;
}

struct ResourceConfig {
    int cpu;
    std::string memory;
    std::string disk;
};

struct MachineConfig {
    std::string hostname;
    std::string ip;
    ResourceConfig resources;
};

struct EnvironmentConfig {
    std::map<std::string, std::string> variables;
};

struct ProcessConfig {
    std::string name;
    std::string machine;
    std::string executable;
    int port;
    EnvironmentConfig environment;
    std::vector<std::string> depends_on;
};

struct ConnectionConfig {
    std::string from;
    std::string to;
    std::string type;
    std::map<std::string, int> config;
};

struct OverlayConfig {
    std::vector<ConnectionConfig> connections;
};

struct DeploymentSettingsConfig {
    std::vector<std::string> start_order;
    std::map<std::string, int> health_checks;
    std::map<std::string, std::string> logging;
    std::map<std::string, std::string> restart_policy;
};

struct DeploymentConfig {
    std::string name;
    std::string version;
    std::string description;
    std::map<std::string, MachineConfig> machines;
    std::map<std::string, ProcessConfig> processes;
    OverlayConfig overlay;
    DeploymentSettingsConfig deployment_settings;
};

DeploymentConfig parseConfig(const std::string& configFile);

void displayConfig(const DeploymentConfig& config);

std::vector<std::string> getPeerAddresses(const DeploymentConfig& config, const std::string& selfProcessId);