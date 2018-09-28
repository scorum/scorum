#pragma once

#include <fc/filesystem.hpp>
#include <fstream>

#include <snapshot/snapshot_fmt.hpp>

namespace chainbase {
class db_state;
}

namespace scorum {

namespace snapshot {

using chainbase::db_state;

class load_algorithm
{
public:
    load_algorithm(db_state&, const fc::path&);

    snapshot_header load_header();

    void load();

private:
    snapshot_header load_header(std::ifstream& fs);

    db_state& _state;
    const fc::path& _snapshot_path;
};
}
}
