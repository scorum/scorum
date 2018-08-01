#include <scorum/protocol/betting/wincase.hpp>
#include <scorum/protocol/betting/wincase_serialization.hpp>
#include <scorum/utils/static_variant_serialization.hpp>

namespace fc {

using namespace scorum;
using namespace scorum::protocol::betting;

void to_variant(const wincase_type& wincase, fc::variant& var)
{
    utils::to_variant(wincase, var);
}
void from_variant(const fc::variant& var, wincase_type& wincase)
{
    utils::from_variant(var, wincase);
}
}