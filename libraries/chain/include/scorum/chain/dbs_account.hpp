#pragma once

#include <scorum/chain/dbs_base_impl.hpp>

#include <scorum/chain/account_object.hpp>

namespace scorum {
namespace chain {

// DB operations with account_*** objects
//
class dbs_account : public dbs_base
{
    friend class dbservice_dbs_factory;

protected:
    explicit dbs_account(database& db);

public:
    const account_object& get_account(const account_name_type&) const;

    const account_authority_object& get_account_authority(const account_name_type&) const;

    void check_account_existence(const account_name_type&,
                                 const optional<const char*>& context_type_name = optional<const char*>()) const;

    void check_account_existence(const account_authority_map&,
                                 const optional<const char*>& context_type_name = optional<const char*>()) const;

    const account_object& create_initial_account(const account_name_type& new_account_name,
                                                 const public_key_type& memo_key,
                                                 const asset& balance_in_scorums,
                                                 const account_name_type& recovery_account,
                                                 const std::string& json_metadata);

    const account_object& create_account(const account_name_type& new_account_name,
                                         const account_name_type& creator_name,
                                         const public_key_type& memo_key,
                                         const std::string& json_metadata,
                                         const authority& owner,
                                         const authority& active,
                                         const authority& posting,
                                         const asset& fee_in_scorums);

    const account_object& create_account_with_delegation(const account_name_type& new_account_name,
                                                         const account_name_type& creator_name,
                                                         const public_key_type& memo_key,
                                                         const std::string& json_metadata,
                                                         const authority& owner,
                                                         const authority& active,
                                                         const authority& posting,
                                                         const asset& fee_in_scorums,
                                                         const asset& delegation_in_vests,
                                                         const optional<time_point_sec>& now
                                                         = optional<time_point_sec>());

    void update_acount(const account_object& account,
                       const account_authority_object& account_authority,
                       const public_key_type& memo_key,
                       const std::string& json_metadata,
                       const optional<authority>& owner,
                       const optional<authority>& active,
                       const optional<authority>& posting,
                       const optional<time_point_sec>& now = optional<time_point_sec>());

    void increase_balance(const account_object& account, const asset& scorums);
    void decrease_balance(const account_object& account, const asset& scorums);

    void increase_reward_balance(const account_object& account, const asset& scorums);
    void decrease_reward_balance(const account_object& account, const asset& scorums);

    void increase_vesting_shares(const account_object& account, const asset& vesting, const asset& scorums);

    void increase_delegated_vesting_shares(const account_object& account, const asset& vesting);

    void increase_received_vesting_shares(const account_object& account, const asset& vesting);
    void decrease_received_vesting_shares(const account_object& account, const asset& vesting);

    void drop_challenged(const account_object& account,
                         const optional<time_point_sec>& now = optional<time_point_sec>());

    void prove_authority(const account_object& account,
                         bool require_owner,
                         const optional<time_point_sec>& now = optional<time_point_sec>());

    void update_withdraw(const account_object& account,
                         const asset& vesting,
                         const time_point_sec& next_vesting_withdrawal,
                         const share_type& to_withdrawn,
                         const optional<share_type>& withdrawn = optional<share_type>());

    void increase_withdraw_routes(const account_object& account);
    void decrease_withdraw_routes(const account_object& account);

    void increase_witnesses_voted_for(const account_object& account);
    void decrease_witnesses_voted_for(const account_object& account);

    void add_post(const account_object& author_account,
                  const optional<account_name_type>& parent_author_name,
                  const optional<time_point_sec>& now = optional<time_point_sec>());

    void update_voting_power(const account_object& account,
                             uint16_t voting_power,
                             const optional<time_point_sec>& now = optional<time_point_sec>());

    void update_owner_authority(const account_object& account,
                                const authority& owner_authority,
                                const optional<time_point_sec>& now = optional<time_point_sec>());

    void create_account_recovery(const account_name_type& account_to_recover_name,
                                 const authority& new_owner_authority,
                                 const optional<time_point_sec>& now = optional<time_point_sec>());

    void submit_account_recovery(const account_object& account_to_recover,
                                 const authority& new_owner_authority,
                                 const authority& recent_owner_authority,
                                 const optional<time_point_sec>& now = optional<time_point_sec>());

    void change_recovery_account(const account_object& account_to_recover,
                                 const account_name_type& new_recovery_account,
                                 const optional<time_point_sec>& now = optional<time_point_sec>());

    void update_voting_proxy(const account_object& account, const optional<account_object>& proxy_account);

    /**
     * @param to_account - the account to receive the new vesting shares
     * @param SCORUM - SCORUM to be converted to vesting shares
     * @param to_reward_balance
     * @return the sbd created and deposited to_account, may return SCORUM if there is no median feed
     */
    asset create_vesting(const account_object& to_account, const asset& scorum, bool to_reward_balance = false);

    /** clears all vote records for a particular account but does not update the
     * witness vote totals.  Vote totals should be updated first via a call to
     * adjust_proxied_witness_votes( a, -a.witness_vote_weight() )
     */
    void clear_witness_votes(const account_object& account);

    /** this updates the votes for witnesses as a result of account voting proxy changing */
    void adjust_proxied_witness_votes(const account_object& account,
                                      const std::array<share_type, SCORUM_MAX_PROXY_RECURSION_DEPTH + 1>& delta,
                                      int depth = 0);

    /** this updates the votes for all witnesses as a result of account VESTS changing */
    void adjust_proxied_witness_votes(const account_object& account, const share_type& delta, int depth = 0);

private:
    const account_object& get_account(const account_id_type&) const;
};
} // namespace chain
} // namespace scorum
