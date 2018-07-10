#include <boost/test/unit_test.hpp>

#include "budget_check_common.hpp"

#include <scorum/chain/services/advertising_property_service.hpp>
#include <scorum/chain/services/budgets.hpp>

#include <scorum/chain/schema/advertising_property_object.hpp>

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
        , advertising_property_service(db.advertising_property_service())
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

    template <typename ServiceType> void winners_arranging_test(ServiceType& service, const budget_type type)
    {
        BOOST_MESSAGE("Create budgets winner list: bob (1), alice (2), sam (3), sam (4), .., kenny(-1)");

        auto start = budget_start + budget_start_interval;

        create_budget(alice, type, budget_balance, start, budget_deadline);
        create_budget(bob, type, budget_balance * 2, start, budget_deadline);
        create_budget(sam, type, budget_balance / 2, start, budget_deadline);

        auto top_count = advertising_property_service.get().vcg_post_coefficients.size();
        for (size_t ci = 0; ci < top_count - 3; ++ci)
        {
            create_budget(zorro, type, budget_balance / 2, start, budget_deadline);
        }

        create_budget(kenny, type, budget_balance / 3, start, budget_deadline);

        generate_blocks(start);

        BOOST_REQUIRE_EQUAL(service.get_budgets().size(), top_count + 1);

        BOOST_CHECK_GT(budget_visitor.get_advertising_summ(bob.name), budget_visitor.get_advertising_summ(alice.name));
        BOOST_CHECK_GT(budget_visitor.get_advertising_summ(alice.name), budget_visitor.get_advertising_summ(sam.name));

        BOOST_CHECK_EQUAL(budget_visitor.get_advertising_summ(alice.name)
                              + budget_visitor.get_cashback_summ(alice.name),
                          get_single_budget(service, alice.name).per_block);
        BOOST_CHECK_EQUAL(budget_visitor.get_advertising_summ(bob.name) + budget_visitor.get_cashback_summ(bob.name),
                          get_single_budget(service, bob.name).per_block);

        BOOST_CHECK_EQUAL(budget_visitor.get_advertising_summ(kenny.name), ASSET_NULL_SCR);
        BOOST_CHECK_EQUAL(budget_visitor.get_cashback_summ(kenny.name),
                          get_single_budget(service, kenny.name).per_block);
    }

    template <typename ServiceType>
    void winners_less_then_top_arranging_test(ServiceType& service, const budget_type type)
    {
        auto start = budget_start + budget_start_interval;

        create_budget(alice, type, budget_balance, start, budget_deadline);
        create_budget(bob, type, budget_balance * 2, start, budget_deadline);
        create_budget(sam, type, budget_balance * 3, start, budget_deadline);

        generate_blocks(start);

        BOOST_REQUIRE_EQUAL(service.get_budgets().size(), 3);

        BOOST_CHECK_EQUAL(budget_visitor.get_advertising_summ(bob.name),
                          budget_visitor.get_advertising_summ(alice.name));
        BOOST_CHECK_GT(budget_visitor.get_advertising_summ(sam.name), budget_visitor.get_advertising_summ(bob.name));

        BOOST_CHECK_EQUAL(budget_visitor.get_advertising_summ(alice.name)
                              + budget_visitor.get_cashback_summ(alice.name),
                          get_single_budget(service, alice.name).per_block);
        BOOST_CHECK_EQUAL(budget_visitor.get_advertising_summ(bob.name) + budget_visitor.get_cashback_summ(bob.name),
                          get_single_budget(service, bob.name).per_block);
    }

    account_service_i& account_service;
    advertising_property_service_i& advertising_property_service;

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

SCORUM_TEST_CASE(no_winnerse_to_arrange_for_any_budget_types_check)
{
    auto start = budget_start + budget_start_interval;
    auto deadline = start + budget_start_interval;

    create_budget(alice, budget_type::post, budget_balance, start, deadline);

    BOOST_REQUIRE_NO_THROW(generate_blocks(start));

    BOOST_CHECK(!post_budget_service.get_budgets(alice.name).empty());
    BOOST_CHECK(banner_budget_service.get_budgets(alice.name).empty());

    BOOST_REQUIRE_NO_THROW(generate_blocks(deadline));

    BOOST_CHECK(post_budget_service.get_budgets(alice.name).empty());
    BOOST_CHECK(banner_budget_service.get_budgets(alice.name).empty());

    start = deadline + budget_start_interval;
    deadline = start + budget_start_interval;

    create_budget(alice, budget_type::banner, budget_balance, start, deadline);

    BOOST_REQUIRE_NO_THROW(generate_blocks(start));

    BOOST_CHECK(post_budget_service.get_budgets(alice.name).empty());
    BOOST_CHECK(!banner_budget_service.get_budgets(alice.name).empty());

    BOOST_REQUIRE_NO_THROW(generate_blocks(deadline));

    BOOST_CHECK(post_budget_service.get_budgets(alice.name).empty());
    BOOST_CHECK(banner_budget_service.get_budgets(alice.name).empty());
}

SCORUM_TEST_CASE(post_budget_from_same_acc_arranging)
{
    /*
     * VCG algorithm for 3 bets/2 coefficients:
     *
     * CPB3=Bid3 -- this one is not handled by VCG algorithm. This is our decision
     * CPB2=Bid3
     * CPB1=(X2*Bid3+(X1-X2)*Bid2)/X1
     * -------------------------------------------------------
     *
     * VCG algorithm for 2 bets/1 coefficients [current test case]:
     * CPB2=Bid2 -- this one is not handled by VCG algorithm. This is our decision
     * CPB1=Bid2
     *
     * NOTE: winner's per-block payment equals looser's bet.
     */
    auto b1 = create_budget(alice, budget_type::post, 100, 10);
    generate_block();

    {
        auto budgets = post_budget_service.get_budgets();
        BOOST_REQUIRE_EQUAL(budgets.size(), 1);
        // There is a single budget so whole 'per-block' amount should be decreased from budget
        BOOST_CHECK_EQUAL(budgets[0].get().balance.amount, 100 - budgets[0].get().per_block.amount);
        BOOST_CHECK_EQUAL(fc::to_string(budgets[0].get().json_metadata), b1.json_metadata);
    }

    auto b2 = create_budget(alice, budget_type::post, 200, 10);
    generate_block();

    {
        auto budgets = post_budget_service.get_budgets();
        BOOST_REQUIRE_EQUAL(budgets.size(), 2);
        // 'b1' is a loser
        BOOST_CHECK_EQUAL(budgets[0].get().balance.amount, 100 - 2 * budgets[0].get().per_block.amount);
        BOOST_CHECK_EQUAL(fc::to_string(budgets[0].get().json_metadata), b1.json_metadata);
        // There are two budgets. 'b2' is a winner. According to VCG algorithm its per-block payment should be equal to
        // looser's per-block payment
        BOOST_CHECK_EQUAL(budgets[1].get().balance.amount, 200 - budgets[1].get().per_block.amount);
        BOOST_CHECK_EQUAL(fc::to_string(budgets[1].get().json_metadata), b2.json_metadata);
    }
}

SCORUM_TEST_CASE(post_budget_winners_arranging_check)
{
    winners_arranging_test(post_budget_service, budget_type::post);
}

SCORUM_TEST_CASE(banner_budget_winners_arranging_check)
{
    winners_arranging_test(banner_budget_service, budget_type::banner);
}

SCORUM_TEST_CASE(post_budget_winners_less_then_top_arranging_check)
{
    winners_less_then_top_arranging_test(post_budget_service, budget_type::post);
}

SCORUM_TEST_CASE(bunner_budget_winners_less_then_top_arranging_check)
{
    winners_less_then_top_arranging_test(banner_budget_service, budget_type::banner);
}

BOOST_AUTO_TEST_SUITE_END()
}
