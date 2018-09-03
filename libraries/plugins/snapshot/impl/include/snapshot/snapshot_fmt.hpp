#pragma once

#include <fc/log/logger.hpp>

#include <scorum/protocol/types.hpp>

#include <snapshot/config.hpp>

namespace scorum {
namespace snapshot {

using scorum::protocol::digest_type;

struct snapshot_header
{
    uint8_t version = 0;
    uint32_t head_block_number = 0;
    digest_type head_block_digest;
};
}
}

FC_REFLECT(scorum::snapshot::snapshot_header, (version)(head_block_number)(head_block_digest))

#define snapshot_log(CTX, FORMAT, ...) fc_ctx_dlog(fc::logger::get("snapshot"), CTX, FORMAT, __VA_ARGS__)
