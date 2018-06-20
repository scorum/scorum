#include <scorum/utils/string_conv.hpp>
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
}
}
