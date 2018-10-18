#pragma once

namespace chainbase {
class segment_manager;
template <typename segment_manager> class database_index;
}

namespace scorum {
namespace chain {
namespace dba {

using db_index = chainbase::database_index<chainbase::segment_manager>;

} // dba
} // chain
} // scorum
