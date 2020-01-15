//
// Created by System Administrator on 2020/1/15.
//

#include "Context.h"
#include <yaml-cpp/yaml.h>
#include "../utils/YamlHelper.h"


bool Context::Init() {
    YamlHelper h;
    YAML::Node config = YAML::LoadFile("config.yaml");



    const std::string username = config["username"].as<std::string>();
    const std::string password = config["password"].as<std::string>();

}