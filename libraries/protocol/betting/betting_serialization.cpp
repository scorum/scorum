#include <scorum/protocol/betting/betting_serialization.hpp>
#include <scorum/utils/static_variant_serialization.hpp>

namespace scorum {
namespace utils {
template <>
template <typename TVariantItem>
std::string static_variant_convertor<protocol::wincase_type>::get_type_name(const TVariantItem& obj) const
{
    std::string type_name = fc::get_typename<TVariantItem>::name();
    auto nested_type_threshold = type_name.find_last_of(':') - 2;
    auto type_threshold = type_name.find_last_of(':', nested_type_threshold) + 1;
    return type_name.substr(type_threshold, type_name.size() - type_threshold);
}
}
}

namespace fc {

using namespace scorum;

void to_variant(const wincase_type& wincase, fc::variant& var)
{
    utils::to_variant(wincase, var);
}
void from_variant(const fc::variant& var, wincase_type& wincase)
{
    utils::from_variant(var, wincase);
}

void to_variant(const market_type& market, fc::variant& var)
{
    utils::to_variant(market, var);
}
void from_variant(const fc::variant& var, market_type& market)
{
    utils::from_variant(var, market);
}

void to_variant(const game_type& game, fc::variant& var)
{
    utils::to_variant(game, var);
}
void from_variant(const fc::variant& var, game_type& game)
{
    utils::from_variant(var, game);
}
}
