//
// Created by System Administrator on 2020/1/15.
//

#include "YamlHelper.h"
#include <spdlog/spdlog.h>

YamlHelper::YamlHelper() {
    try {
        this->root = std::make_unique<YAML::Node>(YAML::LoadFile("config.yaml"));
    }catch (std::exception& e) {
        SPDLOG_INFO("config.yaml file not found or malformed");
    }
}