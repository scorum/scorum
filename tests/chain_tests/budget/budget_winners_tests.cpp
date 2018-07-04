#include <boost/test/unit_test.hpp>

#include "budget_check_common.hpp"

#include <scorum/chain/services/development_committee.hpp>

#include <scorum/chain/schema/dev_committee_object.hpp>

#include <map>

namespace budget_winners_tests {

using namespace budget_check_common;

struct budget_payout_visitor
{
    typedef void result_type;

    budget_payout_visitor(database& db)
        : _db(db)
    {
    }

    using account_stat_type = std::map<account_name_type, share_type>;

    asset get_advertising_summ(const account_name_type& name)
    {
        return asset(_adv_summ[name], SCORUM_SYMBOL);
    }

    asset get_cashback_summ(const account_name_type& name)
    {
        return asset(_cashback_summ[name], SCORUM_SYMBOL);
    }

    void operator()(const allocate_cash_from_advertising_budget_operation& op)
    {
        BOOST_REQUIRE(op.cash.symbol() == SCORUM_SYMBOL);
        _adv_summ[op.owner] += op.cash.amount;
    }

    void operator()(const cash_back_from_advertising_budget_to_owner_operation& op)
    {
        BOOST_REQUIRE(op.cash.symbol() == SCORUM_SYMBOL);
        _cashback_summ[op.owner] += op.cash.amount;
    }

    template <typename Op> void operator()(Op&&) const
    {
    }

private:
    account_stat_type _adv_summ;
    account_stat_type _cashback_summ;
    database& _db;
};

class budget_winners_tests_fixture : public budget_check_fixture
{
public:
    budget_winners_tests_fixture()
        : account_service(db.account_service())
        , development_committee_service(db.development_committee_service())
        , budget_visitor(db)
        , alice("alice")
        , bob("bob")
        , sam("sam")
        , zorro("zorro")
        , kenny("kenny")
    {
        actor(initdelegate).create_account(alice);
        actor(initdelegate).give_scr(alice, BUDGET_BALANCE_DEFAULT * 1000000);
        actor(initdelegate).create_account(bob);
        actor(initdelegate).give_scr(bob, BUDGET_BALANCE_DEFAULT * 2000000);
        actor(initdelegate).create_account(sam);
        actor(initdelegate).give_scr(sam, BUDGET_BALANCE_DEFAULT * 3000000);
        actor(initdelegate).create_account(zorro);
        actor(initdelegate).give_scr(zorro, BUDGET_BALANCE_DEFAULT * 1000000);
        actor(initdelegate).create_account(kenny);
        actor(initdelegate).give_scr(kenny, BUDGET_BALANCE_DEFAULT * 1000000);

        budget_balance = account_service.get_account(alice.name).balance / 10;
        budget_start = db.head_block_time();
        budget_deadline = budget_start + fc::hours(1);

        db.post_apply_operation.connect([&](const operation_notification& note) { note.op.visit(budget_visitor); });
    }

    template <typename ServiceType>
    const typename ServiceType::object_type& get_single_budget(ServiceType& service, const account_name_type name)
    {
        using opject_type = typename ServiceType::object_type;
        auto budgets = service.get_budgets(name);
        const auto& it = budgets.begin();
        BOOST_REQUIRE(it != budgets.end());
        const opject_type& budget = (*it);
        return budget;
    }

    account_service_i& account_service;
    development_committee_service_i& development_committee_service;

    const fc::microseconds budget_start_interval = fc::seconds(SCORUM_BLOCK_INTERVAL * 22);

    asset budget_balance;
    fc::time_point_sec budget_start;
    fc::time_point_sec budget_deadline;

    budget_payout_visitor budget_visitor;

    Actor alice;
    Actor bob;
    Actor sam;
    Actor zorro;
    Actor kenny;
};

BOOST_FIXTURE_TEST_SUITE(budget_winners_check, budget_winners_tests_fixture)

SCORUM_TEST_CASE(top_budgets_win_check)
{
    BOOST_MESSAGE("Create budgets winner list: bob (1), alice (2), sam (3), sam (4), .., kenny(-1)");

    auto start = budget_start + budget_start_interval;

    create_budget(alice, budget_type::post, budget_balance, start, budget_deadline);
    create_budget(bob, budget_type::post, budget_balance * 2, start, budget_deadline);
    create_budget(sam, budget_type::post, budget_balance / 2, start, budget_deadline);

    auto top_count = development_committee_service.get().vcg_post_coefficients.size();
    for (size_t ci = 0; ci < top_count - 3; ++ci)
    {
        create_budget(zorro, budget_type::post, budget_balance / 2, start, budget_deadline);
    }

    create_budget(kenny, budget_type::post, budget_balance / 3, start, budget_deadline);

    generate_blocks(start, false);

    BOOST_REQUIRE_EQUAL(post_budget_service.get_budgets().size(), top_count + 1);

    BOOST_CHECK_GT(budget_visitor.get_advertising_summ(bob.name), budget_visitor.get_advertising_summ(alice.name));
    BOOST_CHECK_GT(budget_visitor.get_advertising_summ(alice.name), budget_visitor.get_advertising_summ(sam.name));

    BOOST_CHECK_EQUAL(budget_visitor.get_advertising_summ(alice.name) + budget_visitor.get_cashback_summ(alice.name),
                      get_single_budget(post_budget_service, alice.name).per_block);
    BOOST_CHECK_EQUAL(budget_visitor.get_advertising_summ(bob.name) + budget_visitor.get_cashback_summ(bob.name),
                      get_single_budget(post_budget_service, bob.name).per_block);

    BOOST_CHECK_EQUAL(budget_visitor.get_advertising_summ(kenny.name), ASSET_NULL_SCR);
    BOOST_CHECK_EQUAL(budget_visitor.get_cashback_summ(kenny.name),
                      get_single_budget(post_budget_service, kenny.name).per_block);
}

BOOST_AUTO_TEST_SUITE_END()
}
