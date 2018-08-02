#include <scorum/protocol/betting/wincase.hpp>

namespace fc {
using namespace scorum::protocol::betting;

class variant;

void to_variant(const wincase_type& wincase, fc::variant& variant);
void from_variant(const fc::variant& variant, wincase_type& wincase);
}