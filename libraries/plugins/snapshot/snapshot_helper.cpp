#include <scorum/snapshot/snapshot_helper.hpp>

#include <sstream>

#include <scorum/chain/services/dynamic_global_property.hpp>

namespace scorum {
namespace snapshot {

fc::path create_snapshot_path(chain::dynamic_global_property_service_i& dprops_service, const fc::path& snapshot_dir)
{
    std::stringstream snapshot_name;
    snapshot_name << dprops_service.get().time.to_iso_string();
    snapshot_name << "-";
    snapshot_name << dprops_service.get().head_block_number;
    snapshot_name << ".bin";

    return snapshot_dir / snapshot_name.str();
}
}
}
