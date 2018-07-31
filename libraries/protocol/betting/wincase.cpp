#include <scorum/protocol/betting/wincase.hpp>
#include <fc/variant.hpp>

namespace fc {
void to_variant(const scorum::protocol::betting::wincase_type& wincase, fc::variant& variant)
{
    variant = wincase.visit([](const auto& w) { return fc::get_typename<std::decay_t<decltype(w)>>::name(); });
}
}