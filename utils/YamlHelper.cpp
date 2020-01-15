//
// Created by System Administrator on 2020/1/15.
//

#include "YamlHelper.h"
YamlHelper::YamlHelper() {
    this->root = std::make_unique<YAML::Node>(YAML::LoadFile("config.yaml"));
}