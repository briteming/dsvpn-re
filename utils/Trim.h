#pragma once

#include <string>

std::string&& TrimNewline(std::string&& str) {
    auto len = str.length();
    if (len <= 0) return std::move(str);
    if (str[len - 1] == '\n') {
        auto pos = str.rfind ("\n",len);
        if (pos != std::string::npos){
            str.erase ( pos, 1 );
        }
    }
    return std::move(str);
}