#include <scorum/protocol/betting/market_kind.hpp>
#include <fc/variant.hpp>

namespace fc {
void to_variant(const scorum::protocol::market_kind& m, fc::variant& variant)
{
    variant = fc::reflector<scorum::protocol::market_kind>::to_string(m);
}
}
