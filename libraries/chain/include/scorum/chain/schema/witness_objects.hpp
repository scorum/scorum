#pragma once

#include <scorum/protocol/authority.hpp>

#include <scorum/chain/schema/scorum_object_types.hpp>

#include <boost/multi_index/composite_key.hpp>

namespace scorum {
namespace chain {

using scorum::protocol::asset;
using scorum::protocol::asset_symbol_type;
using scorum::protocol::chain_properties;
using scorum::protocol::digest_type;
using scorum::protocol::hardfork_version;
using scorum::protocol::public_key_type;
using scorum::protocol::version;

/**
 *  All witnesses with at least 1% net positive approval and
 *  at least 2 weeks old are able to participate in block
 *  production.
 */
class witness_object : public object<witness_object_type, witness_object>
{
    witness_object() = delete;

public:
    enum witness_schedule_type
    {
        top20,
        timeshare,
        none
    };

    template <typename Constructor, typename Allocator>
    witness_object(Constructor&& c, allocator<Allocator> a)
        : url(a)
    {
        c(*this);
    }

    id_type id;

    /** the account that has authority over this witness */
    account_name_type owner;
    time_point_sec created;
    fc::shared_string url;
    uint32_t total_missed = 0;
    uint64_t last_confirmed_block_num = 0;

    /**
     *  This is the key used to sign blocks on behalf of this witness
     */
    public_key_type signing_key;

    chain_properties proposed_chain_props;

    /**
     *  The total votes for this witness. This determines how the witness is ranked for
     *  scheduling.  The top N witnesses by votes are scheduled every round, every one
     *  else takes turns being scheduled proportional to their votes.
     */
    share_type votes;
    witness_schedule_type schedule = none; /// How the witness was scheduled the last time it was scheduled

    /**
     * These fields are used for the witness scheduling algorithm which uses
     * virtual time to ensure that all witnesses are given proportional time
     * for producing blocks.
     *
     * @ref votes is used to determine speed. The @ref virtual_scheduled_time is
     * the expected time at which this witness should complete a virtual lap which
     * is defined as the position equal to 1000 times MAXVOTES.
     *
     * virtual_scheduled_time = virtual_last_update + (1000*MAXVOTES - virtual_position) / votes
     *
     * Every time the number of votes changes the virtual_position and virtual_scheduled_time must
     * update.  There is a global current virtual_scheduled_time which gets updated every time
     * a witness is scheduled.  To update the virtual_position the following math is performed.
     *
     * virtual_position       = virtual_position + votes * (virtual_current_time - virtual_last_update)
     * virtual_last_update    = virtual_current_time
     * votes                  += delta_vote
     * virtual_scheduled_time = virtual_last_update + (1000*MAXVOTES - virtual_position) / votes
     *
     * @defgroup virtual_time Virtual Time Scheduling
     */
    ///@{
    fc::uint128 virtual_last_update;
    fc::uint128 virtual_position;
    fc::uint128 virtual_scheduled_time = fc::uint128::max_value();
    ///@}

    /**
     * This field represents the Scorum blockchain version the witness is running.
     */
    version running_version;

    hardfork_version hardfork_version_vote;
    time_point_sec hardfork_time_vote;
};

class witness_vote_object : public object<witness_vote_object_type, witness_vote_object>
{
public:
    CHAINBASE_DEFAULT_CONSTRUCTOR(witness_vote_object)

    id_type id;

    witness_id_type witness;
    account_id_type account;
};

class witness_schedule_object : public object<witness_schedule_object_type, witness_schedule_object>
{
public:
    CHAINBASE_DEFAULT_CONSTRUCTOR(witness_schedule_object)

    id_type id;

    fc::uint128 current_virtual_time;
    fc::array<account_name_type, SCORUM_MAX_WITNESSES> current_shuffled_witnesses;
    uint8_t num_scheduled_witnesses = 1;
};

struct by_vote_name;
struct by_name;
struct by_pow;
struct by_schedule_time;
/**
 * @ingroup object_index
 */
typedef multi_index_container<witness_object,
                              indexed_by<ordered_unique<tag<by_id>,
                                                        member<witness_object, witness_id_type, &witness_object::id>>,
                                         ordered_unique<tag<by_name>,
                                                        member<witness_object,
                                                               account_name_type,
                                                               &witness_object::owner>>,
                                         ordered_unique<tag<by_vote_name>,
                                                        composite_key<witness_object,
                                                                      member<witness_object,
                                                                             share_type,
                                                                             &witness_object::votes>,
                                                                      member<witness_object,
                                                                             account_name_type,
                                                                             &witness_object::owner>>,
                                                        composite_key_compare<std::greater<share_type>,
                                                                              std::less<account_name_type>>>,
                                         ordered_unique<tag<by_schedule_time>,
                                                        composite_key<witness_object,
                                                                      member<witness_object,
                                                                             fc::uint128,
                                                                             &witness_object::virtual_scheduled_time>,
                                                                      member<witness_object,
                                                                             witness_id_type,
                                                                             &witness_object::id>>>>,
                              allocator<witness_object>>
    witness_index;

struct by_account_witness;
struct by_witness_account;
typedef multi_index_container<witness_vote_object,
                              indexed_by<ordered_unique<tag<by_id>,
                                                        member<witness_vote_object,
                                                               witness_vote_id_type,
                                                               &witness_vote_object::id>>,
                                         ordered_unique<tag<by_account_witness>,
                                                        composite_key<witness_vote_object,
                                                                      member<witness_vote_object,
                                                                             account_id_type,
                                                                             &witness_vote_object::account>,
                                                                      member<witness_vote_object,
                                                                             witness_id_type,
                                                                             &witness_vote_object::witness>>,
                                                        composite_key_compare<std::less<account_id_type>,
                                                                              std::less<witness_id_type>>>,
                                         ordered_unique<tag<by_witness_account>,
                                                        composite_key<witness_vote_object,
                                                                      member<witness_vote_object,
                                                                             witness_id_type,
                                                                             &witness_vote_object::witness>,
                                                                      member<witness_vote_object,
                                                                             account_id_type,
                                                                             &witness_vote_object::account>>,
                                                        composite_key_compare<std::less<witness_id_type>,
                                                                              std::
                                                                                  less<account_id_type>>>>, // indexed_by
                              allocator<witness_vote_object>>
    witness_vote_index;

typedef multi_index_container<witness_schedule_object,
                              indexed_by<ordered_unique<tag<by_id>,
                                                        member<witness_schedule_object,
                                                               witness_schedule_id_type,
                                                               &witness_schedule_object::id>>>,
                              allocator<witness_schedule_object>>
    witness_schedule_index;
} // namespace chain
} // namespace scorum

// clang-format off

FC_REFLECT_ENUM( scorum::chain::witness_object::witness_schedule_type, (top20)(timeshare)(none) )

FC_REFLECT( scorum::chain::witness_object,
             (id)
             (owner)
             (created)
             (url)(votes)(schedule)(virtual_last_update)(virtual_position)(virtual_scheduled_time)(total_missed)
             (last_confirmed_block_num)(signing_key)
             (proposed_chain_props)
             (running_version)
             (hardfork_version_vote)(hardfork_time_vote)
          )
CHAINBASE_SET_INDEX_TYPE( scorum::chain::witness_object, scorum::chain::witness_index )

FC_REFLECT( scorum::chain::witness_vote_object, (id)(witness)(account) )
CHAINBASE_SET_INDEX_TYPE( scorum::chain::witness_vote_object, scorum::chain::witness_vote_index )

FC_REFLECT( scorum::chain::witness_schedule_object,
             (id)(current_virtual_time)(current_shuffled_witnesses)(num_scheduled_witnesses)
          )
CHAINBASE_SET_INDEX_TYPE( scorum::chain::witness_schedule_object, scorum::chain::witness_schedule_index )

// clang-format on
