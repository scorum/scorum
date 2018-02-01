#include <scorum/protocol/asset.hpp>
#include <boost/rational.hpp>
#include <boost/multiprecision/cpp_int.hpp>

/*

The bounds on asset serialization are as follows:

index : field
0     : decimals
1..6  : symbol
   7  : \0
*/

namespace scorum {
namespace protocol {
typedef boost::multiprecision::int128_t int128_t;

void asset::_check_symbol()
{
    FC_ASSERT(SCORUM_SYMBOL == _symbol || VESTS_SYMBOL == _symbol,
              "Invalid asset symbol received. ${1} not either ${2} or ${3}.",
              ("1", _symbol)("2", SCORUM_SYMBOL)("3", VESTS_SYMBOL));
}

void asset::_set_decimals(uint8_t d)
{
    FC_ASSERT(d < 15);
    auto a = (char*)&_symbol;
    a[0] = d;
}

uint8_t asset::decimals() const
{
    auto a = (const char*)&_symbol;
    uint8_t result = uint8_t(a[0]);
    FC_ASSERT(result < 15);
    return result;
}

std::string asset::symbol_name() const
{
    auto a = (const char*)&_symbol;
    FC_ASSERT(a[7] == 0);
    return &a[1];
}

int64_t asset::precision() const
{
    static int64_t table[] = { 1,
                               10,
                               100,
                               1000,
                               10000,
                               100000,
                               1000000,
                               10000000,
                               100000000ll,
                               1000000000ll,
                               10000000000ll,
                               100000000000ll,
                               1000000000000ll,
                               10000000000000ll,
                               100000000000000ll };
    uint8_t d = decimals();
    return table[d];
}

std::string asset::to_string() const
{
    int64_t prec = precision();
    std::string result = fc::to_string(amount.value / prec);
    if (prec > 1)
    {
        auto fract = amount.value % prec;
        // prec is a power of ten, so for example when working with
        // 7.005 we have fract = 5, prec = 1000.  So prec+fract=1005
        // has the correct number of zeros and we can simply trim the
        // leading 1.
        result += "." + fc::to_string(prec + fract).erase(0, 1);
    }
    return result + " " + symbol_name();
}

asset asset::from_string(const std::string& from)
{
    try
    {
        std::string s = fc::trim(from);
        auto space_pos = s.find(" ");
        auto dot_pos = s.find(".");

        FC_ASSERT(space_pos != std::string::npos);

        asset result;
        result._symbol = uint64_t(0);
        auto sy = (char*)&result._symbol;

        if (dot_pos != std::string::npos)
        {
            FC_ASSERT(space_pos > dot_pos);

            auto intpart = s.substr(0, dot_pos);
            auto fractpart = "1" + s.substr(dot_pos + 1, space_pos - dot_pos - 1);
            result._set_decimals(fractpart.size() - 1);

            result.amount = fc::to_int64(intpart);
            result.amount.value *= result.precision();
            result.amount.value += fc::to_int64(fractpart);
            result.amount.value -= result.precision();
        }
        else
        {
            auto intpart = s.substr(0, space_pos);
            result.amount = fc::to_int64(intpart);
            result._set_decimals(0);
        }
        auto symbol = s.substr(space_pos + 1);
        size_t symbol_size = symbol.size();

        if (symbol_size > 0)
        {
            FC_ASSERT(symbol_size <= 6);
            memcpy(sy + 1, symbol.c_str(), symbol_size);
        }

        result._check_symbol();
        return result;
    }
    FC_CAPTURE_AND_RETHROW((from))
}
}
} // scorum::protocol
