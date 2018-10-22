#pragma once

namespace scorum {

namespace chain {
struct genesis_state_type;
}

namespace util {

using scorum::chain::genesis_state_type;

void test_database(const genesis_state_type&, unsigned int shared_mem_mb_size);
}
}
