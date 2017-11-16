#pragma once

#include <memory>

#include <scorum/chain/dbs_base_impl.hpp>

namespace scorum {
namespace chain {

class account_object;
class account_authority_object;

// DB operations with account_*** objects
//
class dbs_account : public dbs_base
{
public:
    explicit dbs_account(database& db);

public:
    void create_account_by_faucets(const account_name_type& new_account_name,
                                   const account_name_type& creator_name,
                                   const public_key_type& memo_key,
                                   const string& json_metadata,
                                   const authority& owner,
                                   const authority& active,
                                   const authority& posting,
                                   const asset& fee);

    void create_account_with_delegation(const account_name_type& new_account_name,
                                        const account_name_type& creator_name,
                                        const public_key_type& memo_key,
                                        const string& json_metadata,
                                        const authority& owner,
                                        const authority& active,
                                        const authority& posting,
                                        const asset& fee,
                                        const asset& delegation);

    const account_object& get_account(const account_name_type&) const;

    const account_authority_object& get_account_authority(const account_name_type&) const;

    void update_acount(const account_object& account,
                       const account_authority_object& account_authority,
                       const public_key_type& memo_key,
                       const string& json_metadata,
                       const optional<authority> &owner,
                       const optional<authority> &active,
                       const optional<authority> &posting);

    void update_owner_authority(const account_object& account, const authority& owner_authority);

    void check_account_existence(const account_name_type&) const;

    void check_account_existence(const account_authority_map&) const;

    void prove_authority(const account_object& challenged, bool require_owner);


};
}
}
