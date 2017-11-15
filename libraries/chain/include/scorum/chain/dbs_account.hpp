#pragma once

#include <memory>

#include <scorum/chain/dbservice_common.hpp>

namespace scorum {
namespace chain {

    class dbs_account
    {
    public:
        explicit dbs_account(dbservice& db);

        typedef std::unique_ptr<dbs_account> ptr;

    public:

        void write_account_creation_by_faucets(
                               const account_name_type& new_account_name,
                               const account_name_type& creator_name,
                               const public_key_type &memo_key,
                               const string &json_metadata,
                               const shared_authority& owner,
                               const shared_authority& active,
                               const shared_authority& posting,
                               const asset &fee);

        void write_account_creation_with_delegation(
                               const account_name_type& new_account_name,
                               const account_name_type& creator_name,
                               const public_key_type &memo_key,
                               const string &json_metadata,
                               const shared_authority& owner,
                               const shared_authority& active,
                               const shared_authority& posting,
                               const asset &fee,
                               const asset &delegation);

    private:

        database &_db;
    };

}
}
