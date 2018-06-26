#include <scorum/chain/evaluators/scorum_evaluators.hpp>

#include <scorum/rewards_math/curve.hpp>

#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/witness.hpp>
#include <scorum/chain/services/witness_vote.hpp>
#include <scorum/chain/services/comment.hpp>
#include <scorum/chain/services/comment_vote.hpp>
#include <scorum/chain/services/registration_pool.hpp>
#include <scorum/chain/services/registration_committee.hpp>
#include <scorum/chain/services/atomicswap.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/services/escrow.hpp>
#include <scorum/chain/services/decline_voting_rights_request.hpp>
#include <scorum/chain/services/scorumpower_delegation.hpp>
#include <scorum/chain/services/reward_funds.hpp>
#include <scorum/chain/services/withdraw_scorumpower.hpp>
#include <scorum/chain/services/account_blogging_statistic.hpp>
#include <scorum/chain/services/comment_statistic.hpp>

#include <scorum/chain/data_service_factory.hpp>

#include <scorum/rewards_math/formulas.hpp>

#include <scorum/chain/schema/account_objects.hpp>
#include <scorum/chain/schema/atomicswap_objects.hpp>
#include <scorum/chain/schema/scorum_objects.hpp>
#include <scorum/chain/schema/witness_objects.hpp>

#ifndef IS_LOW_MEM
#include <diff_match_patch.h>
#include <boost/locale/encoding_utf.hpp>

using boost::locale::conv::utf_to_utf;

std::wstring utf8_to_wstring(const std::string& str)
{
    return utf_to_utf<wchar_t>(str.c_str(), str.c_str() + str.size());
}

std::string wstring_to_utf8(const std::wstring& str)
{
    return utf_to_utf<char>(str.c_str(), str.c_str() + str.size());
}

#endif

#include <fc/uint128.hpp>
#include <fc/utf8.hpp>

#include <limits>

namespace scorum {
namespace chain {
using fc::uint128_t;

inline void validate_permlink_0_1(const std::string& permlink)
{
    FC_ASSERT(permlink.size() > SCORUM_MIN_PERMLINK_LENGTH && permlink.size() < SCORUM_MAX_PERMLINK_LENGTH,
              "Permlink is not a valid size.");

    for (auto ch : permlink)
    {
        if (!std::islower(ch) && !std::isdigit(ch) && !(ch == '-'))
        {
            FC_ASSERT(false, "Invalid permlink character: ${ch}", ("ch", std::string(1, ch)));
        }
    }
}

struct strcmp_equal
{
    bool operator()(const fc::shared_string& a, const std::string& b)
    {
        return a.size() == b.size() && std::strcmp(a.c_str(), b.c_str()) == 0;
    }
};

void witness_update_evaluator::do_apply(const witness_update_operation& o)
{
    account_service_i& account_service = db().account_service();
    witness_service_i& witness_service = db().witness_service();

    account_service.check_account_existence(o.owner);

    FC_ASSERT(o.url.size() <= SCORUM_MAX_WITNESS_URL_LENGTH, "URL is too long");
    FC_ASSERT(o.proposed_chain_props.account_creation_fee.symbol() == SCORUM_SYMBOL);

    if (!witness_service.is_exists(o.owner))
    {
        witness_service.create_witness(o.owner, o.url, o.block_signing_key, o.proposed_chain_props);
    }
    else
    {
        const auto& witness = witness_service.get(o.owner);
        witness_service.update_witness(witness, o.url, o.block_signing_key, o.proposed_chain_props);
    }
}

void account_create_evaluator::do_apply(const account_create_operation& o)
{
    account_service_i& account_service = db().account_service();
    dynamic_global_property_service_i& dprops_service = db().dynamic_global_property_service();

    const auto& creator = account_service.get_account(o.creator);

    // check creator balance

    FC_ASSERT(creator.balance >= o.fee, "Insufficient balance to create account.",
              ("creator.balance", creator.balance)("required", o.fee));

    // check fee

    const auto creation_fee
        = dprops_service.get().median_chain_props.account_creation_fee * SCORUM_CREATE_ACCOUNT_WITH_SCORUM_MODIFIER;
    FC_ASSERT(o.fee >= creation_fee, "Insufficient Fee: ${f} required, ${p} provided.",
              ("f", creation_fee)("p", o.fee));

    // check accounts existence

    account_service.check_account_existence(o.owner.account_auths);

    account_service.check_account_existence(o.active.account_auths);

    account_service.check_account_existence(o.posting.account_auths);

    // write in to DB

    account_service.create_account(o.new_account_name, o.creator, o.memo_key, o.json_metadata, o.owner, o.active,
                                   o.posting, o.fee);
}

void account_create_with_delegation_evaluator::do_apply(const account_create_with_delegation_operation& o)
{
    account_service_i& account_service = db().account_service();
    dynamic_global_property_service_i& dprops_service = db().dynamic_global_property_service();
    withdraw_scorumpower_service_i& withdraw_scorumpower_service = db().withdraw_scorumpower_service();

    const auto& creator = account_service.get_account(o.creator);

    // check creator balance

    FC_ASSERT(creator.balance >= o.fee, "Insufficient balance to create account.",
              ("creator.balance", creator.balance)("required", o.fee));

    // check delegation fee

    asset withdraw_rest = withdraw_scorumpower_service.get_withdraw_rest(creator.id);

    FC_ASSERT(creator.scorumpower - creator.delegated_scorumpower - withdraw_rest >= o.delegation,
              "Insufficient scorumpower to delegate to new account.",
              ("creator.scorumpower", creator.scorumpower)("creator.delegated_scorumpower",
                                                           creator.delegated_scorumpower)("required", o.delegation));

    const auto median_creation_fee = dprops_service.get().median_chain_props.account_creation_fee;

    auto target_delegation = asset(median_creation_fee.amount * SCORUM_CREATE_ACCOUNT_WITH_SCORUM_MODIFIER
                                       * SCORUM_CREATE_ACCOUNT_DELEGATION_RATIO,
                                   SP_SYMBOL);

    auto current_delegation = asset(o.fee.amount * SCORUM_CREATE_ACCOUNT_DELEGATION_RATIO, SP_SYMBOL) + o.delegation;

    FC_ASSERT(current_delegation >= target_delegation, "Inssufficient Delegation ${f} required, ${p} provided.",
              ("f", target_delegation)("p", current_delegation)("account_creation_fee", median_creation_fee)(
                  "o.fee", o.fee)("o.delegation", o.delegation));

    FC_ASSERT(o.fee >= median_creation_fee, "Insufficient Fee: ${f} required, ${p} provided.",
              ("f", median_creation_fee)("p", o.fee));

    // check accounts existence

    account_service.check_account_existence(o.owner.account_auths);

    account_service.check_account_existence(o.active.account_auths);

    account_service.check_account_existence(o.posting.account_auths);

    account_service.create_account_with_delegation(o.new_account_name, o.creator, o.memo_key, o.json_metadata, o.owner,
                                                   o.active, o.posting, o.fee, o.delegation);
}

void account_update_evaluator::do_apply(const account_update_operation& o)
{
    if (o.posting)
        o.posting->validate();

    account_service_i& account_service = db().account_service();

    const auto& account = account_service.get_account(o.account);
    const auto& account_auth = account_service.get_account_authority(o.account);

    if (o.owner)
    {
        dynamic_global_property_service_i& dprops_service = db().dynamic_global_property_service();

        FC_ASSERT(dprops_service.head_block_time() - account_auth.last_owner_update > SCORUM_OWNER_UPDATE_LIMIT,
                  "Owner authority can only be updated once an hour.");

        account_service.check_account_existence(o.owner->account_auths);

        account_service.update_owner_authority(account, *o.owner);
    }

    if (o.active)
    {
        account_service.check_account_existence(o.active->account_auths);
    }

    if (o.posting)
    {
        account_service.check_account_existence(o.posting->account_auths);
    }

    account_service.update_acount(account, account_auth, o.memo_key, o.json_metadata, o.owner, o.active, o.posting);
}

/**
 *  Because net_rshares is 0 there is no need to update any pending payout calculations or parent posts.
 */
void delete_comment_evaluator::do_apply(const delete_comment_operation& o)
{
    account_service_i& account_service = db().account_service();
    comment_service_i& comment_service = db().comment_service();
    comment_vote_service_i& comment_vote_service = db().comment_vote_service();
    dynamic_global_property_service_i& dprops_service = db().dynamic_global_property_service();

    const auto& auth = account_service.get_account(o.author);
    FC_ASSERT(!(auth.owner_challenged || auth.active_challenged),
              "Operation cannot be processed because account is currently challenged.");

    const auto& comment = comment_service.get(o.author, o.permlink);
    FC_ASSERT(comment.children == 0, "Cannot delete a comment with replies.");

    FC_ASSERT(comment.cashout_time != fc::time_point_sec::maximum());

    FC_ASSERT(comment.net_rshares <= 0, "Cannot delete a comment with net positive votes.");

    if (comment.net_rshares > 0)
        return;

    comment_vote_service.remove_by_comment(comment_id_type(comment.id));

    account_name_type parent_author = comment.parent_author;
    std::string parent_permlink = fc::to_string(comment.parent_permlink);
    /// this loop can be skiped for validate-only nodes as it is merely gathering stats for indices
    while (parent_author != SCORUM_ROOT_POST_PARENT_ACCOUNT)
    {
        const comment_object& parent = comment_service.get(parent_author, parent_permlink);

        parent_author = parent.parent_author;
        parent_permlink = fc::to_string(parent.parent_permlink);

        auto now = dprops_service.head_block_time();

        comment_service.update(parent, [&](comment_object& p) {
            p.children--;
            p.active = now;
        });

#ifdef IS_LOW_MEM
        break;
#endif
    }

    comment_service.remove(comment);
}

struct comment_options_extension_visitor
{
    comment_options_extension_visitor(const comment_object& c, data_service_factory_i& db)
        : _c(c)
        , _account_service(db.account_service())
        , _comment_service(db.comment_service())
    {
    }

    void operator()(const comment_payout_beneficiaries& cpb) const
    {
        FC_ASSERT(_c.beneficiaries.size() == 0, "Comment already has beneficiaries specified.");
        FC_ASSERT(_c.abs_rshares == 0, "Comment must not have been voted on before specifying beneficiaries.");

        _comment_service.update(_c, [&](comment_object& c) {
            for (auto& b : cpb.beneficiaries)
            {
                _account_service.check_account_existence(b.account, "Beneficiary");
                c.beneficiaries.push_back(b);
            }
        });
    }

    typedef void result_type;

    const comment_object& _c;
    account_service_i& _account_service;
    comment_service_i& _comment_service;
};

void comment_options_evaluator::do_apply(const comment_options_operation& o)
{
    account_service_i& account_service = db().account_service();
    comment_service_i& comment_service = db().comment_service();

    const auto& auth = account_service.get_account(o.author);
    FC_ASSERT(!(auth.owner_challenged || auth.active_challenged),
              "Operation cannot be processed because account is currently challenged.");

    const auto& comment = comment_service.get(o.author, o.permlink);
    if (!o.allow_curation_rewards || !o.allow_votes || o.max_accepted_payout < comment.max_accepted_payout)
        FC_ASSERT(comment.abs_rshares == 0,
                  "One of the included comment options requires the comment to have no rshares allocated to it.");

    FC_ASSERT(comment.allow_curation_rewards >= o.allow_curation_rewards, "Curation rewards cannot be re-enabled.");
    FC_ASSERT(comment.allow_votes >= o.allow_votes, "Voting cannot be re-enabled.");
    FC_ASSERT(comment.max_accepted_payout >= o.max_accepted_payout, "A comment cannot accept a greater payout.");

    comment_service.update(comment, [&](comment_object& c) {
        c.max_accepted_payout = o.max_accepted_payout;
        c.allow_votes = o.allow_votes;
        c.allow_curation_rewards = o.allow_curation_rewards;
    });

    for (auto& e : o.extensions)
    {
        e.visit(comment_options_extension_visitor(comment, db()));
    }
}

void comment_evaluator::do_apply(const comment_operation& o)
{
    account_service_i& account_service = db().account_service();
    comment_service_i& comment_service = db().comment_service();
    comment_statistic_scr_service_i& comment_statistic_scr_service = db().comment_statistic_scr_service();
    comment_statistic_sp_service_i& comment_statistic_sp_service = db().comment_statistic_sp_service();
    dynamic_global_property_service_i& dprops_service = db().dynamic_global_property_service();

    try
    {

        FC_ASSERT(o.title.size() + o.body.size() + o.json_metadata.size(),
                  "Cannot update comment because nothing appears to be changing.");

        const auto& auth = account_service.get_account(o.author); /// prove it exists

        FC_ASSERT(!(auth.owner_challenged || auth.active_challenged),
                  "Operation cannot be processed because account is currently challenged.");

        account_name_type parent_author = o.parent_author;
        std::string parent_permlink = o.parent_permlink;
        if (parent_author != SCORUM_ROOT_POST_PARENT_ACCOUNT)
        {
            const comment_object& parent = comment_service.get(parent_author, parent_permlink);
            FC_ASSERT(parent.depth < SCORUM_MAX_COMMENT_DEPTH,
                      "Comment is nested ${x} posts deep, maximum depth is ${y}.",
                      ("x", parent.depth)("y", SCORUM_MAX_COMMENT_DEPTH));
        }

        auto now = dprops_service.head_block_time();

        if (!comment_service.is_exists(o.author, o.permlink))
        {
            if (parent_author != SCORUM_ROOT_POST_PARENT_ACCOUNT)
            {
                const comment_object& parent = comment_service.get(parent_author, parent_permlink);
                FC_ASSERT(comment_service.get(parent.root_comment).allow_replies,
                          "The parent comment has disabled replies.");
            }

            if (parent_author == SCORUM_ROOT_POST_PARENT_ACCOUNT)
                FC_ASSERT((now - auth.last_root_post) > SCORUM_MIN_ROOT_COMMENT_INTERVAL,
                          "You may only post once every 5 minutes.",
                          ("now", now)("last_root_post", auth.last_root_post));
            else
                FC_ASSERT((now - auth.last_post) > SCORUM_MIN_REPLY_INTERVAL,
                          "You may only comment once every 20 seconds.",
                          ("now", now)("auth.last_post", auth.last_post));

            account_service.add_post(auth, parent_author);

            validate_permlink_0_1(parent_permlink);
            validate_permlink_0_1(o.permlink);

            account_name_type pr_parent_author;
            std::string pr_parent_permlink;
            uint16_t pr_depth = 0;
            std::string pr_category;
            comment_id_type pr_root_comment;
            if (parent_author != SCORUM_ROOT_POST_PARENT_ACCOUNT)
            {
                const comment_object& parent = comment_service.get(parent_author, parent_permlink);
                pr_parent_author = parent.author;
                pr_parent_permlink = fc::to_string(parent.permlink);
                pr_depth = parent.depth;
                pr_category = fc::to_string(parent.category);
                pr_root_comment = parent.root_comment;
            }

            const comment_object& new_comment = comment_service.create([&](comment_object& com) {

                com.author = o.author;
                fc::from_string(com.permlink, o.permlink);
                com.last_update = now;
                com.created = com.last_update;
                com.active = com.last_update;
                com.last_payout = fc::time_point_sec();

                if (parent_author == SCORUM_ROOT_POST_PARENT_ACCOUNT)
                {
                    com.parent_author = "";
                    fc::from_string(com.parent_permlink, parent_permlink);
                    fc::from_string(com.category, parent_permlink);
                    com.root_comment = com.id;
                }
                else
                {
                    com.parent_author = pr_parent_author;
                    fc::from_string(com.parent_permlink, pr_parent_permlink);
                    com.depth = pr_depth + 1;
                    fc::from_string(com.category, pr_category);
                    com.root_comment = pr_root_comment;
                }

                com.cashout_time = com.created + SCORUM_CASHOUT_WINDOW_SECONDS;

#ifndef IS_LOW_MEM
                fc::from_string(com.title, o.title);
                if (o.body.size() < 1024 * 1024 * 128)
                {
                    fc::from_string(com.body, o.body);
                }

                fc::from_string(com.json_metadata, o.json_metadata);
#endif
            });

            comment_statistic_scr_service.create(
                [&](comment_statistic_scr_object& stat) { stat.comment = new_comment.id; });
            comment_statistic_sp_service.create(
                [&](comment_statistic_sp_object& stat) { stat.comment = new_comment.id; });

#ifndef IS_LOW_MEM
            {
                account_blogging_statistic_service_i& account_blogging_statistic_service
                    = db().account_blogging_statistic_service();

                const auto& author_stat = account_blogging_statistic_service.obtain(auth.id);
                account_blogging_statistic_service.add_post(author_stat);
                if (parent_author != SCORUM_ROOT_POST_PARENT_ACCOUNT)
                {
                    account_blogging_statistic_service.add_comment(author_stat);
                }
            }
#endif
            /// this loop can be skiped for validate-only nodes as it is merely gathering stats for indices
            while (parent_author != SCORUM_ROOT_POST_PARENT_ACCOUNT)
            {
                const comment_object& parent = comment_service.get(parent_author, parent_permlink);

                parent_author = parent.parent_author;
                parent_permlink = fc::to_string(parent.parent_permlink);

                comment_service.update(parent, [&](comment_object& p) {
                    p.children++;
                    p.active = now;
                });
#ifdef IS_LOW_MEM
                break;
#endif
            }
        }
        else // start edit case
        {
            const comment_object& comment = comment_service.get(o.author, o.permlink);

            comment_service.update(comment, [&](comment_object& com) {
                com.last_update = dprops_service.head_block_time();
                com.active = com.last_update;
                strcmp_equal equal;

                if (parent_author == SCORUM_ROOT_POST_PARENT_ACCOUNT)
                {
                    FC_ASSERT(com.parent_author == account_name_type(), "The parent of a comment cannot be changed.");
                    FC_ASSERT(equal(com.parent_permlink, parent_permlink), "The permlink of a comment cannot change.");
                }
                else
                {
                    FC_ASSERT(com.parent_author == o.parent_author, "The parent of a comment cannot be changed.");
                    FC_ASSERT(equal(com.parent_permlink, parent_permlink), "The permlink of a comment cannot change.");
                }

#ifndef IS_LOW_MEM
                if (o.title.size())
                    fc::from_string(com.title, o.title);
                if (!o.json_metadata.empty())
                {
                    fc::from_string(com.json_metadata, o.json_metadata);
                }

                if (!o.body.empty())
                {
                    try
                    {
                        diff_match_patch<std::wstring> dmp;
                        auto patch = dmp.patch_fromText(utf8_to_wstring(o.body));
                        if (patch.size())
                        {
                            auto result = dmp.patch_apply(patch, utf8_to_wstring(fc::to_string(com.body)));
                            auto patched_body = wstring_to_utf8(result.first);
                            if (!fc::is_utf8(patched_body))
                            {
                                idump(("invalid utf8")(patched_body));
                                fc::from_string(com.body, fc::prune_invalid_utf8(patched_body));
                            }
                            else
                            {
                                fc::from_string(com.body, patched_body);
                            }
                        }
                        else
                        { // replace
                            fc::from_string(com.body, o.body);
                        }
                    }
                    catch (...)
                    {
                        fc::from_string(com.body, o.body);
                    }
                }
#endif
            });

        } // end EDIT case
    }
    FC_CAPTURE_AND_RETHROW((o))
}

void escrow_transfer_evaluator::do_apply(const escrow_transfer_operation& o)
{
    account_service_i& account_service = db().account_service();
    dynamic_global_property_service_i& dprops_service = db().dynamic_global_property_service();
    escrow_service_i& escrow_service = db().escrow_service();

    try
    {
        const auto& from_account = account_service.get_account(o.from);
        account_service.check_account_existence(o.to);
        account_service.check_account_existence(o.agent);

        FC_ASSERT(o.ratification_deadline > dprops_service.head_block_time(),
                  "The escorw ratification deadline must be after head block time.");
        FC_ASSERT(o.escrow_expiration > dprops_service.head_block_time(),
                  "The escrow expiration must be after head block time.");

        FC_ASSERT(o.fee.symbol() == SCORUM_SYMBOL, "Fee must be in SCR.");

        asset scorum_spent = o.scorum_amount;
        scorum_spent += o.fee;

        FC_ASSERT(from_account.balance >= scorum_spent,
                  "Account cannot cover SCR costs of escrow. Required: ${r} Available: ${a}",
                  ("r", scorum_spent)("a", from_account.balance));

        account_service.decrease_balance(from_account, scorum_spent);

        escrow_service.create_escrow(o.escrow_id, o.from, o.to, o.agent, o.ratification_deadline, o.escrow_expiration,
                                     o.scorum_amount, o.fee);
    }
    FC_CAPTURE_AND_RETHROW((o))
}

void escrow_approve_evaluator::do_apply(const escrow_approve_operation& o)
{
    account_service_i& account_service = db().account_service();
    dynamic_global_property_service_i& dprops_service = db().dynamic_global_property_service();
    escrow_service_i& escrow_service = db().escrow_service();

    try
    {

        const auto& escrow = escrow_service.get(o.from, o.escrow_id);

        FC_ASSERT(escrow.to == o.to, "Operation 'to' (${o}) does not match escrow 'to' (${e}).",
                  ("o", o.to)("e", escrow.to));
        FC_ASSERT(escrow.agent == o.agent, "Operation 'agent' (${a}) does not match escrow 'agent' (${e}).",
                  ("o", o.agent)("e", escrow.agent));
        FC_ASSERT(escrow.ratification_deadline >= dprops_service.head_block_time(),
                  "The escrow ratification deadline has passed. Escrow can no longer be ratified.");

        bool reject_escrow = !o.approve;

        if (o.who == o.to)
        {
            FC_ASSERT(!escrow.to_approved, "Account 'to' (${t}) has already approved the escrow.", ("t", o.to));

            if (!reject_escrow)
            {
                escrow_service.update(escrow, [&](escrow_object& esc) { esc.to_approved = true; });
            }
        }
        if (o.who == o.agent)
        {
            FC_ASSERT(!escrow.agent_approved, "Account 'agent' (${a}) has already approved the escrow.",
                      ("a", o.agent));

            if (!reject_escrow)
            {
                escrow_service.update(escrow, [&](escrow_object& esc) { esc.agent_approved = true; });
            }
        }

        if (reject_escrow)
        {
            const auto& from_account = account_service.get_account(o.from);
            account_service.increase_balance(from_account, escrow.scorum_balance);
            account_service.increase_balance(from_account, escrow.pending_fee);

            escrow_service.remove(escrow);
        }
        else if (escrow.to_approved && escrow.agent_approved)
        {
            const auto& agent_account = account_service.get_account(o.agent);
            account_service.increase_balance(agent_account, escrow.pending_fee);

            escrow_service.update(escrow, [&](escrow_object& esc) { esc.pending_fee.amount = 0; });
        }
    }
    FC_CAPTURE_AND_RETHROW((o))
}

void escrow_dispute_evaluator::do_apply(const escrow_dispute_operation& o)
{
    account_service_i& account_service = db().account_service();
    dynamic_global_property_service_i& dprops_service = db().dynamic_global_property_service();
    escrow_service_i& escrow_service = db().escrow_service();

    try
    {
        account_service.check_account_existence(o.from);

        const auto& e = escrow_service.get(o.from, o.escrow_id);
        FC_ASSERT(dprops_service.head_block_time() < e.escrow_expiration,
                  "Disputing the escrow must happen before expiration.");
        FC_ASSERT(e.to_approved && e.agent_approved,
                  "The escrow must be approved by all parties before a dispute can be raised.");
        FC_ASSERT(!e.disputed, "The escrow is already under dispute.");
        FC_ASSERT(e.to == o.to, "Operation 'to' (${o}) does not match escrow 'to' (${e}).", ("o", o.to)("e", e.to));
        FC_ASSERT(e.agent == o.agent, "Operation 'agent' (${a}) does not match escrow 'agent' (${e}).",
                  ("o", o.agent)("e", e.agent));

        escrow_service.update(e, [&](escrow_object& esc) { esc.disputed = true; });
    }
    FC_CAPTURE_AND_RETHROW((o))
}

void escrow_release_evaluator::do_apply(const escrow_release_operation& o)
{
    account_service_i& account_service = db().account_service();
    dynamic_global_property_service_i& dprops_service = db().dynamic_global_property_service();
    escrow_service_i& escrow_service = db().escrow_service();

    try
    {
        account_service.check_account_existence(o.from);
        const auto& receiver_account = account_service.get_account(o.receiver);

        const auto& e = escrow_service.get(o.from, o.escrow_id);
        FC_ASSERT(e.scorum_balance >= o.scorum_amount,
                  "Release amount exceeds escrow balance. Amount: ${a}, Balance: ${b}",
                  ("a", o.scorum_amount)("b", e.scorum_balance));
        FC_ASSERT(e.to == o.to, "Operation 'to' (${o}) does not match escrow 'to' (${e}).", ("o", o.to)("e", e.to));
        FC_ASSERT(e.agent == o.agent, "Operation 'agent' (${a}) does not match escrow 'agent' (${e}).",
                  ("o", o.agent)("e", e.agent));
        FC_ASSERT(o.receiver == e.from || o.receiver == e.to, "Funds must be released to 'from' (${f}) or 'to' (${t})",
                  ("f", e.from)("t", e.to));
        FC_ASSERT(e.to_approved && e.agent_approved, "Funds cannot be released prior to escrow approval.");

        // If there is a dispute regardless of expiration, the agent can release funds to either party
        if (e.disputed)
        {
            FC_ASSERT(o.who == e.agent, "Only 'agent' (${a}) can release funds in a disputed escrow.", ("a", e.agent));
        }
        else
        {
            FC_ASSERT(o.who == e.from || o.who == e.to,
                      "Only 'from' (${f}) and 'to' (${t}) can release funds from a non-disputed escrow",
                      ("f", e.from)("t", e.to));

            if (e.escrow_expiration > dprops_service.head_block_time())
            {
                // If there is no dispute and escrow has not expired, either party can release funds to the other.
                if (o.who == e.from)
                {
                    FC_ASSERT(o.receiver == e.to, "Only 'from' (${f}) can release funds to 'to' (${t}).",
                              ("f", e.from)("t", e.to));
                }
                else if (o.who == e.to)
                {
                    FC_ASSERT(o.receiver == e.from, "Only 'to' (${t}) can release funds to 'from' (${t}).",
                              ("f", e.from)("t", e.to));
                }
            }
        }
        // If escrow expires and there is no dispute, either party can release funds to either party.

        account_service.increase_balance(receiver_account, o.scorum_amount);

        escrow_service.update(e, [&](escrow_object& esc) { esc.scorum_balance -= o.scorum_amount; });

        if (e.scorum_balance.amount == 0)
        {
            escrow_service.remove(e);
        }
    }
    FC_CAPTURE_AND_RETHROW((o))
}

void transfer_evaluator::do_apply(const transfer_operation& o)
{
    account_service_i& account_service = db().account_service();

    const auto& from_account = account_service.get_account(o.from);
    const auto& to_account = account_service.get_account(o.to);

    account_service.drop_challenged(from_account);

    FC_ASSERT(from_account.balance >= o.amount, "Account does not have sufficient funds for transfer.");
    account_service.decrease_balance(from_account, o.amount);
    account_service.increase_balance(to_account, o.amount);
}

void transfer_to_scorumpower_evaluator::do_apply(const transfer_to_scorumpower_operation& o)
{
    account_service_i& account_service = db().account_service();

    const auto& from_account = account_service.get_account(o.from);
    const auto& to_account = o.to.size() ? account_service.get_account(o.to) : from_account;

    FC_ASSERT(from_account.balance >= o.amount, "Account does not have sufficient SCR for transfer.");
    account_service.decrease_balance(from_account, o.amount);
    account_service.create_scorumpower(to_account, o.amount);
}

void account_witness_proxy_evaluator::do_apply(const account_witness_proxy_operation& o)
{
    account_service_i& account_service = db().account_service();

    const auto& account = account_service.get_account(o.account);
    FC_ASSERT(account.proxy != o.proxy, "Proxy must change.");

    FC_ASSERT(account.can_vote, "Account has declined the ability to vote and cannot proxy votes.");

    optional<account_object> proxy;
    if (o.proxy.size())
    {
        proxy = account_service.get_account(o.proxy);
    }
    account_service.update_voting_proxy(account, proxy);
}

void account_witness_vote_evaluator::do_apply(const account_witness_vote_operation& o)
{
    account_service_i& account_service = db().account_service();
    witness_service_i& witness_service = db().witness_service();
    witness_vote_service_i& witness_vote_service = db().witness_vote_service();

    const auto& voter = account_service.get_account(o.account);
    FC_ASSERT(voter.proxy.size() == 0, "A proxy is currently set, please clear the proxy before voting for a witness.");

    if (o.approve)
        FC_ASSERT(voter.can_vote, "Account has declined its voting rights.");

    const auto& witness = witness_service.get(o.witness);

    if (!witness_vote_service.is_exists(witness.id, voter.id))
    {
        FC_ASSERT(o.approve, "Vote doesn't exist, user must indicate a desire to approve witness.");

        FC_ASSERT(voter.witnesses_voted_for < SCORUM_MAX_ACCOUNT_WITNESS_VOTES,
                  "Account has voted for too many witnesses."); // TODO: Remove after hardfork 2

        witness_vote_service.create([&](witness_vote_object& v) {
            v.witness = witness.id;
            v.account = voter.id;
        });

        witness_service.adjust_witness_vote(witness, voter.witness_vote_weight());

        account_service.increase_witnesses_voted_for(voter);
    }
    else
    {
        FC_ASSERT(!o.approve, "Vote currently exists, user must indicate a desire to reject witness.");

        witness_service.adjust_witness_vote(witness, -voter.witness_vote_weight());

        account_service.decrease_witnesses_voted_for(voter);

        const witness_vote_object& witness_vote_object = witness_vote_service.get(witness.id, voter.id);
        witness_vote_service.remove(witness_vote_object);
    }
}

void vote_evaluator::do_apply(const vote_operation& o)
{
    account_service_i& account_service = db().account_service();
    comment_service_i& comment_service = db().comment_service();
    comment_vote_service_i& comment_vote_service = db().comment_vote_service();
    dynamic_global_property_service_i& dprops_service = db().dynamic_global_property_service();

    try
    {
        const auto& comment = comment_service.get(o.author, o.permlink);
        const auto& voter = account_service.get_account(o.voter);

        FC_ASSERT(!(voter.owner_challenged || voter.active_challenged),
                  "Operation cannot be processed because the account is currently challenged.");

        FC_ASSERT(voter.can_vote, "Voter has declined their voting rights.");

        const vote_weight_type weight = o.weight * SCORUM_1_PERCENT;
        if (weight > 0)
            FC_ASSERT(comment.allow_votes, "Votes are not allowed on the comment.");

        if (comment.cashout_time == fc::time_point_sec::maximum())
        {
#ifndef CLEAR_VOTES
            if (!comment_vote_service.is_exists(comment.id, voter.id))
            {
                comment_vote_service.create([&](comment_vote_object& cvo) {
                    cvo.voter = voter.id;
                    cvo.comment = comment.id;
                    cvo.vote_percent = weight;
                    cvo.last_update = dprops_service.head_block_time();
                });
            }
            else
            {
                const comment_vote_object& comment_vote = comment_vote_service.get(comment.id, voter.id);
                comment_vote_service.update(comment_vote, [&](comment_vote_object& cvo) {
                    cvo.vote_percent = weight;
                    cvo.last_update = dprops_service.head_block_time();
                });
            }
#endif
            return;
        }

        {
            int64_t elapsed_seconds = (dprops_service.head_block_time() - voter.last_vote_time).to_seconds();
            FC_ASSERT(elapsed_seconds >= SCORUM_MIN_VOTE_INTERVAL_SEC, "Can only vote once every 3 seconds.");
        }

        uint16_t current_power
            = rewards_math::calculate_restoring_power(voter.voting_power, dprops_service.head_block_time(),
                                                      voter.last_vote_time, SCORUM_VOTE_REGENERATION_SECONDS);
        FC_ASSERT(current_power > 0, "Account currently does not have voting power.");

        uint16_t used_power
            = rewards_math::calculate_used_power(current_power, weight, SCORUM_VOTING_POWER_DECAY_PERCENT);

        FC_ASSERT(used_power <= current_power, "Account does not have enough power to vote.");

        share_type abs_rshares
            = rewards_math::calculate_abs_reward_shares(used_power, voter.effective_scorumpower().amount);

        FC_ASSERT(abs_rshares > SCORUM_VOTE_DUST_THRESHOLD || weight == 0,
                  "Voting weight is too small, please accumulate more voting power or scorum power.");

        /// this is the rshares voting for or against the post
        share_type rshares = weight < 0 ? -abs_rshares : abs_rshares;

        if (!comment_vote_service.is_exists(comment.id, voter.id))
        {
            FC_ASSERT(weight != 0, "Vote weight cannot be 0.");

            if (rshares > 0)
            {
                FC_ASSERT(dprops_service.head_block_time() < comment.cashout_time - SCORUM_UPVOTE_LOCKOUT,
                          "Cannot increase payout within last twelve hours before payout.");
            }

            account_service.update_voting_power(voter, current_power - used_power);

            const auto& root_comment = comment_service.get(comment.root_comment);

            FC_ASSERT(abs_rshares > 0, "Cannot vote with 0 rshares.");

            auto old_vote_rshares = comment.vote_rshares;

            comment_service.update(comment, [&](comment_object& c) {
                c.net_rshares += rshares;
                c.abs_rshares += abs_rshares;
                if (rshares > 0)
                    c.vote_rshares += rshares;
                if (rshares > 0)
                    c.net_votes++;
                else
                    c.net_votes--;
            });

            comment_service.update(root_comment, [&](comment_object& c) { c.children_abs_rshares += abs_rshares; });

            uint64_t max_vote_weight = 0;

            comment_vote_service.create([&](comment_vote_object& cv) {
                cv.voter = voter.id;
                cv.comment = comment.id;
                cv.rshares = rshares;
                cv.vote_percent = weight;
                cv.last_update = dprops_service.head_block_time();

                bool curation_reward_eligible
                    = rshares > 0 && (comment.last_payout == fc::time_point_sec()) && comment.allow_curation_rewards;

                if (curation_reward_eligible)
                {
                    const auto& reward_fund = db().content_reward_fund_scr_service().get();
                    max_vote_weight = rewards_math::calculate_max_vote_weight(comment.vote_rshares, old_vote_rshares,
                                                                              reward_fund.curation_reward_curve);
                    cv.weight = rewards_math::calculate_vote_weight(max_vote_weight, cv.last_update, comment.created,
                                                                    SCORUM_REVERSE_AUCTION_WINDOW_SECONDS);
                }
                else
                {
                    cv.weight = 0;
                }
            });

#ifndef IS_LOW_MEM
            {
                account_blogging_statistic_service_i& account_blogging_statistic_service
                    = db().account_blogging_statistic_service();

                const auto& voter_stat = account_blogging_statistic_service.obtain(voter.id);
                account_blogging_statistic_service.add_vote(voter_stat);
            }
#endif

            if (max_vote_weight) // Optimization
            {
                comment_service.update(comment, [&](comment_object& c) { c.total_vote_weight += max_vote_weight; });
            }
        }
        else
        {
            const comment_vote_object& comment_vote = comment_vote_service.get(comment.id, voter.id);

            FC_ASSERT(comment_vote.num_changes != -1, "Cannot vote again on a comment after payout.");

            FC_ASSERT(comment_vote.num_changes < SCORUM_MAX_VOTE_CHANGES,
                      "Voter has used the maximum number of vote changes on this comment.");

            FC_ASSERT(comment_vote.vote_percent != weight, "You have already voted in a similar way.");

            if (comment_vote.rshares < rshares)
            {
                FC_ASSERT(dprops_service.head_block_time() < comment.cashout_time - SCORUM_UPVOTE_LOCKOUT,
                          "Cannot increase payout within last twelve hours before payout.");
            }

            account_service.update_voting_power(voter, current_power - used_power);

            const auto& root_comment = comment_service.get(comment.root_comment);

            comment_service.update(comment, [&](comment_object& c) {
                c.net_rshares -= comment_vote.rshares;
                c.net_rshares += rshares;
                c.abs_rshares += abs_rshares;

                /// TODO: figure out how to handle remove a vote (rshares == 0 )
                if (rshares > 0 && comment_vote.rshares < 0)
                    c.net_votes += 2;
                else if (rshares > 0 && comment_vote.rshares == 0)
                    c.net_votes += 1;
                else if (rshares == 0 && comment_vote.rshares < 0)
                    c.net_votes += 1;
                else if (rshares == 0 && comment_vote.rshares > 0)
                    c.net_votes -= 1;
                else if (rshares < 0 && comment_vote.rshares == 0)
                    c.net_votes -= 1;
                else if (rshares < 0 && comment_vote.rshares > 0)
                    c.net_votes -= 2;
            });

            comment_service.update(root_comment, [&](comment_object& c) { c.children_abs_rshares += abs_rshares; });

            comment_service.update(comment, [&](comment_object& c) { c.total_vote_weight -= comment_vote.weight; });

            comment_vote_service.update(comment_vote, [&](comment_vote_object& cv) {
                cv.rshares = rshares;
                cv.vote_percent = weight;
                cv.last_update = dprops_service.head_block_time();
                cv.weight = 0;
                cv.num_changes += 1;
            });
        }
    }
    FC_CAPTURE_AND_RETHROW()
}

void prove_authority_evaluator::do_apply(const prove_authority_operation& o)
{
    account_service_i& account_service = db().account_service();

    const auto& challenged = account_service.get_account(o.challenged);
    FC_ASSERT(challenged.owner_challenged || challenged.active_challenged,
              "Account is not challeneged. No need to prove authority.");

    account_service.prove_authority(challenged, o.require_owner);
}

void request_account_recovery_evaluator::do_apply(const request_account_recovery_operation& o)
{
    account_service_i& account_service = db().account_service();
    witness_service_i& witness_service = db().witness_service();

    const auto& account_to_recover = account_service.get_account(o.account_to_recover);

    if (account_to_recover.recovery_account.length()) // Make sure recovery matches expected recovery account
        FC_ASSERT(account_to_recover.recovery_account == o.recovery_account,
                  "Cannot recover an account that does not have you as there recovery partner.");
    else // Empty string recovery account defaults to top witness
        FC_ASSERT(witness_service.get_top_witness().owner == o.recovery_account,
                  "Top witness must recover an account with no recovery partner.");

    account_service.create_account_recovery(o.account_to_recover, o.new_owner_authority);
}

void recover_account_evaluator::do_apply(const recover_account_operation& o)
{
    account_service_i& account_service = db().account_service();
    dynamic_global_property_service_i& dprops_service = db().dynamic_global_property_service();

    const auto& account_to_recover = account_service.get_account(o.account_to_recover);

    FC_ASSERT(dprops_service.head_block_time() - account_to_recover.last_account_recovery > SCORUM_OWNER_UPDATE_LIMIT,
              "Owner authority can only be updated once an hour.");

    account_service.submit_account_recovery(account_to_recover, o.new_owner_authority, o.recent_owner_authority);
}

void change_recovery_account_evaluator::do_apply(const change_recovery_account_operation& o)
{
    account_service_i& account_service = db().account_service();

    account_service.check_account_existence(o.new_recovery_account); // Simply validate account exists
    const auto& account_to_recover = account_service.get_account(o.account_to_recover);

    account_service.change_recovery_account(account_to_recover, o.new_recovery_account);
}

void decline_voting_rights_evaluator::do_apply(const decline_voting_rights_operation& o)
{
    account_service_i& account_service = db().account_service();
    decline_voting_rights_request_service_i& dvrr_service = db().decline_voting_rights_request_service();

    const auto& account = account_service.get_account(o.account);

    if (o.decline)
    {
        FC_ASSERT(!dvrr_service.is_exists(account.id), "Cannot create new request because one already exists.");

        dvrr_service.create_rights(account.id, SCORUM_OWNER_AUTH_RECOVERY_PERIOD);
    }
    else
    {
        const auto& request = dvrr_service.get(account.id);

        dvrr_service.remove(request);
    }
}

void delegate_scorumpower_evaluator::do_apply(const delegate_scorumpower_operation& op)
{
    account_service_i& account_service = db().account_service();
    scorumpower_delegation_service_i& sp_delegation_service = db().scorumpower_delegation_service();
    dynamic_global_property_service_i& dprops_service = db().dynamic_global_property_service();
    withdraw_scorumpower_service_i& withdraw_scorumpower_service = db().withdraw_scorumpower_service();

    const auto& delegator = account_service.get_account(op.delegator);
    const auto& delegatee = account_service.get_account(op.delegatee);

    auto available_shares = delegator.scorumpower - delegator.delegated_scorumpower
        - withdraw_scorumpower_service.get_withdraw_rest(delegator.id);

    const auto& dprops = dprops_service.get();
    auto min_delegation = asset(
        dprops.median_chain_props.account_creation_fee.amount * SCORUM_MIN_DELEGATE_VESTING_SHARES_MODIFIER, SP_SYMBOL);
    auto min_update = asset(dprops.median_chain_props.account_creation_fee.amount, SP_SYMBOL);

    // If delegation doesn't exist, create it
    if (!sp_delegation_service.is_exists(op.delegator, op.delegatee))
    {
        FC_ASSERT(available_shares >= op.scorumpower, "Account does not have enough scorumpower to delegate.");
        FC_ASSERT(op.scorumpower >= min_delegation, "Account must delegate a minimum of ${v}", ("v", min_delegation));

        sp_delegation_service.create([&](scorumpower_delegation_object& spdo) {
            spdo.delegator = op.delegator;
            spdo.delegatee = op.delegatee;
            spdo.scorumpower = op.scorumpower;
            spdo.min_delegation_time = dprops.time;
        });

        account_service.increase_delegated_scorumpower(delegator, op.scorumpower);
        account_service.increase_received_scorumpower(delegatee, op.scorumpower);
    }
    else
    {
        const auto& delegation = sp_delegation_service.get(op.delegator, op.delegatee);

        // Else if the delegation is increasing
        if (op.scorumpower >= delegation.scorumpower)
        {
            auto delta = op.scorumpower - delegation.scorumpower;

            FC_ASSERT(delta >= min_update, "Scorum Power increase is not enough of a difference. min_update: ${min}",
                      ("min", min_update));
            FC_ASSERT(available_shares >= op.scorumpower - delegation.scorumpower,
                      "Account does not have enough scorumpower to delegate.");

            account_service.increase_delegated_scorumpower(delegator, delta);
            account_service.increase_received_scorumpower(delegatee, delta);

            sp_delegation_service.update(delegation,
                                         [&](scorumpower_delegation_object& obj) { obj.scorumpower = op.scorumpower; });
        }
        // Else the delegation is decreasing
        else /* delegation.scorumpower > op.scorumpower */
        {
            auto delta = delegation.scorumpower - op.scorumpower;

            if (op.scorumpower.amount > 0)
            {
                FC_ASSERT(delta >= min_update,
                          "Scorum Power decrease is not enough of a difference. min_update: ${min}",
                          ("min", min_update));
                FC_ASSERT(op.scorumpower >= min_delegation,
                          "Delegation must be removed or leave minimum delegation amount of ${v}",
                          ("v", min_delegation));
            }
            else
            {
                FC_ASSERT(delegation.scorumpower.amount > 0,
                          "Delegation would set scorumpower to zero, but it is already zero");
            }

            sp_delegation_service.create_expiration(
                op.delegator, delta, std::max(dprops_service.head_block_time() + SCORUM_CASHOUT_WINDOW_SECONDS,
                                              delegation.min_delegation_time));

            account_service.decrease_received_scorumpower(delegatee, delta);

            if (op.scorumpower.amount > 0)
            {
                sp_delegation_service.update(
                    delegation, [&](scorumpower_delegation_object& obj) { obj.scorumpower = op.scorumpower; });
            }
            else
            {
                sp_delegation_service.remove(delegation);
            }
        }
    }
}

void atomicswap_initiate_evaluator::do_apply(const atomicswap_initiate_operation& op)
{
    atomicswap_service_i& atomicswap_service = db().atomicswap_service();
    account_service_i& account_service = db().account_service();

    account_service.check_account_existence(op.owner);
    account_service.check_account_existence(op.recipient);

    const auto& owner = account_service.get_account(op.owner);
    const auto& recipient = account_service.get_account(op.recipient);

    optional<std::string> metadata;
    if (!op.metadata.empty())
    {
        metadata = op.metadata;
    }

    switch (op.type)
    {
    case atomicswap_initiate_operation::by_initiator:
        atomicswap_service.create_contract(atomicswap_contract_initiator, owner, recipient, op.amount, op.secret_hash,
                                           metadata);
        break;
    case atomicswap_initiate_operation::by_participant:
        atomicswap_service.create_contract(atomicswap_contract_participant, owner, recipient, op.amount, op.secret_hash,
                                           metadata);
        break;
    default:
        FC_ASSERT(false, "Invalid operation type.");
    }
}

void atomicswap_redeem_evaluator::do_apply(const atomicswap_redeem_operation& op)
{
    atomicswap_service_i& atomicswap_service = db().atomicswap_service();
    account_service_i& account_service = db().account_service();

    account_service.check_account_existence(op.from);
    account_service.check_account_existence(op.to);

    const auto& from = account_service.get_account(op.from);
    const auto& to = account_service.get_account(op.to);

    std::string secret_hash = atomicswap::get_secret_hash(op.secret);

    const auto& contract = atomicswap_service.get_contract(from, to, secret_hash);

    atomicswap_service.redeem_contract(contract, op.secret);
}

void atomicswap_refund_evaluator::do_apply(const atomicswap_refund_operation& op)
{
    atomicswap_service_i& atomicswap_service = db().atomicswap_service();
    account_service_i& account_service = db().account_service();

    account_service.check_account_existence(op.participant);
    account_service.check_account_existence(op.initiator);

    const auto& from = account_service.get_account(op.participant);
    const auto& to = account_service.get_account(op.initiator);

    const auto& contract = atomicswap_service.get_contract(from, to, op.secret_hash);

    FC_ASSERT(contract.type != atomicswap_contract_initiator,
              "Can't refund initiator contract. It is locked on ${h} hours.",
              ("h", SCORUM_ATOMICSWAP_INITIATOR_REFUND_LOCK_SECS / 3600));

    atomicswap_service.refund_contract(contract);
}

} // namespace chain
} // namespace scorum
