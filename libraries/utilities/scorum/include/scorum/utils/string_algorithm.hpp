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

/**
 * Gets valid (in terms of characters) UTF-8 substring from UTF-8 string
 * @param s UTF-8 string to convert
 * @param offset non-negative offset from the beginning of the string. Offset is in symbols not in bytes.
 * @param length non-negative length of substring starting from offset. Length is in symbols not in bytes.
 * @return UTF-8 substring
 */
std::string substring(const std::string& s, unsigned offset, unsigned length);
}
}