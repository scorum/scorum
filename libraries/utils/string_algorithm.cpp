#include <scorum/utils/string_algorithm.hpp>
#include <unicode/unistr.h>

namespace scorum {
namespace utils {

std::string to_lower_copy(const std::string& s)
{
    auto unicode = icu::UnicodeString::fromUTF8(icu::StringPiece(s.c_str()));
    unicode.toLower();
    std::string result;
    unicode.toUTF8String(result);

    return result;
}

std::string substring(const std::string& s, unsigned offset, unsigned length)
{
    auto unicode = icu::UnicodeString::fromUTF8(icu::StringPiece(s.c_str()));
    auto substr_unicode = unicode.tempSubString(offset, length);

    std::string result;
    substr_unicode.toUTF8String(result);

    return result;
}
}
}