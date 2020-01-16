#pragma once

#include "../utils/Singleton.h"
#include <yaml-cpp/yaml.h>
#include <boost/algorithm/string.hpp>
#include <vector>
class YamlHelper : Singleton<YamlHelper> {
public:
    template <class ValueType>
    struct ReturnValue {
        ValueType value;
        bool error = true;
    };

    YamlHelper();

    template <class ValueType>
    ReturnValue<ValueType> Parse(std::string nodes) {
        ReturnValue<ValueType> result;
        std::vector<std::string> node;
        boost::split(node, nodes, boost::is_any_of("."));
        if (node.empty()) return result;
        if (node.size() == 1) {
            try {
                result.value = (*this->root)[node[0]].as<ValueType>();
            }catch (std::exception& e) {
                return result;
            }
            result.error = false;
            return result;
        }

        try {
            YAML::Node subnode;
            for (int i = 0; i < node.size() - 1; i++) {
                auto nodeName = node[i];
                subnode = (*this->root)[nodeName];
            }
            result.value = subnode[node[node.size() - 1]].as<ValueType>();
            result.error = false;
            return result;
        }catch (std::exception &e) {
            return result;
        }
    }

private:
    std::unique_ptr<YAML::Node> root;
};


