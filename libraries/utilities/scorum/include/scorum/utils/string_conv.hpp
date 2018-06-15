#pragma once
#include <string>

namespace scorum {
namespace utils {
    /**
     * Converts UTF-8 string to lower case
     * @param s UTF-8 string to convert
     * @return UTF-8 string in lower case
     */
    std::string to_lower_copy(const std::string& s);
}
}