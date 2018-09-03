#pragma once

#include <fc/filesystem.hpp>

namespace scorum {

namespace chain {
struct dynamic_global_property_service_i;
}

namespace snapshot {

fc::path create_snapshot_path(chain::dynamic_global_property_service_i&, const fc::path& snapshot_dir);
}
}
