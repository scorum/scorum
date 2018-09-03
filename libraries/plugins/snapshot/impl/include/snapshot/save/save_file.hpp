#pragma once

#include <fc/filesystem.hpp>

namespace chainbase {
class db_state;
}

namespace scorum {

namespace protocol {
class signed_block;
}

namespace snapshot {

using db_state = chainbase::db_state;
using signed_block = protocol::signed_block;

class save_algorithm
{
public:
    save_algorithm(db_state&, const signed_block&, const fc::path&);

    void save();

private:
    db_state& _state;
    const signed_block& _block;
    const fc::path& _snapshot_path;
};
}
}
