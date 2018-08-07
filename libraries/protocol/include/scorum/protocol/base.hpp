#pragma once

#include <scorum/protocol/types.hpp>
#include <scorum/protocol/authority.hpp>
#include <scorum/protocol/version.hpp>
#include <scorum/protocol/odds.hpp>

#include <fc/time.hpp>
#include <fc/utf8.hpp>
#include <fc/io/json.hpp>

namespace scorum {
namespace protocol {

inline void validate_account_name(const std::string& name)
{
    FC_ASSERT(is_valid_account_name(name), "Account name ${n} is invalid", ("n", name));
}

inline void validate_permlink(const std::string& permlink)
{
    FC_ASSERT(permlink.size() < SCORUM_MAX_PERMLINK_LENGTH, "permlink is too long");
    FC_ASSERT(fc::is_utf8(permlink), "permlink not formatted in UTF8");
}

inline void validate_json_metadata(const std::string& json_metadata)
{
    if (!json_metadata.empty())
    {
        FC_ASSERT(fc::is_utf8(json_metadata), "JSON Metadata not formatted in UTF8");
        FC_ASSERT(fc::json::is_valid(json_metadata), "JSON Metadata not valid JSON");
    }
}

inline void vallidate_odds(const std::string& odds_value)
{
    try
    {
        odds::from_string(odds_value);
    }
    catch (fc::exception& err)
    {
        elog("${err}", ("err", err.what()));
        FC_ASSERT(false, "Invalid odds value");
    }
}

struct base_operation
{
    void get_required_authorities(std::vector<authority>&) const
    {
    }

    void get_required_active_authorities(flat_set<account_name_type>&) const
    {
    }
    void get_required_posting_authorities(flat_set<account_name_type>&) const
    {
    }

    void get_required_owner_authorities(flat_set<account_name_type>&) const
    {
    }

    bool is_virtual() const
    {
        return false;
    }

    void validate() const
    {
    }
};

struct virtual_operation : public base_operation
{
    bool is_virtual() const
    {
        return true;
    }
    void validate() const
    {
        FC_ASSERT(false, "This is a virtual operation");
    }
};

typedef static_variant<void_t,
                       version, // Normal witness version reporting, for diagnostics and voting
                       hardfork_version_vote // Voting for the next hardfork to trigger
                       >
    block_header_extensions;

typedef static_variant<void_t> future_extensions;

typedef flat_set<block_header_extensions> block_header_extensions_type;
typedef flat_set<future_extensions> extensions_type;
}
} // scorum::protocol

FC_REFLECT_TYPENAME(scorum::protocol::block_header_extensions)
FC_REFLECT_TYPENAME(scorum::protocol::future_extensions)
