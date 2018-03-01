#include "budget_check_common.hpp"

namespace scorum {
namespace chain {

budget_check_fixture::budget_check_fixture()
{
    default_deadline = db.get_slot_time(BLOCK_LIMIT_DEFAULT);
}
}
}
