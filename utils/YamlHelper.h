#pragma once

#include "../utils/Singleton.h"
#include <yaml-cpp/yaml.h>
#include <boost/algorithm/string.hpp>

class YamlHelper : Singleton<YamlHelper> {
public:
    template <class ValueType>
    struct ReturnValue {
        ValueType value;
        bool error;
    };

    YamlHelper();

    template <class ValueType>
    ReturnValue<ValueType> ParseString(std::string node) {
        if (auto sub = config["tun"]) {
            if (sub["local_tun_ip"]) {
                auto value = sub["local_tun_ip"].as<std::string>();
            }
        }
    }

private:
    std::unique_ptr<YAML::Node> root;
};


