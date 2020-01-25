#pragma

#include <string>

class Shell {
public:
    static std::string Run(std::string command);
    static void RunNoResult(std::string command);
};


