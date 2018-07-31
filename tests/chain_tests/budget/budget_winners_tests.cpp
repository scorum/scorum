#include <boost/test/unit_test.hpp>

#include "budget_check_common.hpp"

#include <scorum/chain/services/advertising_property.hpp>
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
        conn = db.post_apply_operation.connect([&](const operation_notification& note) { note.op.visit(*this); });
    }
    ~budget_payout_visitor()
    {
        conn.disconnect();
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

    asset get_last_adv_payment(const account_name_type& name)
    {
        return asset(_last_adv_amount[name], SCORUM_SYMBOL);
    }

    asset get_last_cashback(const account_name_type& name)
    {
        return asset(_last_cashback_amount[name], SCORUM_SYMBOL);
    }

    void operator()(const allocate_cash_from_advertising_budget_operation& op)
    {
        BOOST_REQUIRE(op.cash.symbol() == SCORUM_SYMBOL);
        _adv_summ[op.owner] += op.cash.amount;
        _last_adv_amount[op.owner] = op.cash.amount;
    }

    void operator()(const cash_back_from_advertising_budget_to_owner_operation& op)
    {
        BOOST_REQUIRE(op.cash.symbol() == SCORUM_SYMBOL);
        _cashback_summ[op.owner] += op.cash.amount;
        _last_cashback_amount[op.owner] = op.cash.amount;
    }

    template <typename Op> void operator()(Op&&) const
    {
    }

private:
    account_stat_type _adv_summ;
    account_stat_type _cashback_summ;
    account_stat_type _last_adv_amount;
    account_stat_type _last_cashback_amount;
    database& _db;
    fc::scoped_connection conn;
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
        , cartman("cartman")
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
        actor(initdelegate).create_account(cartman);
        actor(initdelegate).give_scr(cartman, BUDGET_BALANCE_DEFAULT * 1000000);

        budget_balance = account_service.get_account(alice.name).balance / 10;
        budget_start = db.head_block_time();
        budget_deadline = budget_start + fc::hours(1);
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
        BOOST_TEST_MESSAGE("Create budgets winner list: bob (1), alice (2), sam (3), sam (4), .., kenny(-1)");

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

        BOOST_REQUIRE_EQUAL(service.get_budgets().size(), 3u);

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
    Actor cartman;
};

BOOST_FIXTURE_TEST_SUITE(budget_winners_check, budget_winners_tests_fixture)

SCORUM_TEST_CASE(adding_new_budgets_with_increasing_per_block_test)
{
    /*
     * Coefficients: {100, 85, 75, 65}
     */

    // per block
    auto pb = [](const post_budget_object& b) { return b.per_block.amount.value; };

    int deadline = 10;

    {
        budget_payout_visitor v(db);
        create_budget(alice, budget_type::post, 100, deadline);
        generate_block();

        auto bs = post_budget_service.get_budgets();
        BOOST_REQUIRE_EQUAL(bs.size(), 1u);
        auto alice_adv_payment = pb(bs[0]);
        BOOST_CHECK_EQUAL(v.get_last_adv_payment(alice.name).amount, alice_adv_payment);
        BOOST_CHECK_EQUAL(v.get_last_cashback(alice.name).amount, pb(bs[0]) - alice_adv_payment);
    }
    {
        budget_payout_visitor v(db);
        create_budget(bob, budget_type::post, 200, deadline);
        generate_block();

        auto bs = post_budget_service.get_budgets();
        BOOST_REQUIRE_EQUAL(bs.size(), 2u);
        auto alice_adv_payment = pb(bs[0]);
        auto bob_adv_payment = pb(bs[0]);
        BOOST_CHECK_EQUAL(v.get_last_adv_payment(alice.name).amount, alice_adv_payment);
        BOOST_CHECK_EQUAL(v.get_last_adv_payment(bob.name).amount, bob_adv_payment);
        BOOST_CHECK_EQUAL(v.get_last_cashback(alice.name).amount, pb(bs[0]) - alice_adv_payment);
        BOOST_CHECK_EQUAL(v.get_last_cashback(bob.name).amount, pb(bs[1]) - bob_adv_payment);
    }
    {
        budget_payout_visitor v(db);
        create_budget(sam, budget_type::post, 300, deadline);
        generate_block();

        auto bs = post_budget_service.get_budgets();
        BOOST_REQUIRE_EQUAL(bs.size(), 3u);
        auto alice_adv_payment = pb(bs[0]);
        auto bob_adv_payment = pb(bs[0]);
        auto sam_adv_payment = (85 * pb(bs[0]) + 15 * pb(bs[1])) / 100;
        BOOST_CHECK_EQUAL(v.get_last_adv_payment(alice.name).amount, alice_adv_payment);
        BOOST_CHECK_EQUAL(v.get_last_adv_payment(bob.name).amount, bob_adv_payment);
        BOOST_CHECK_EQUAL(v.get_last_adv_payment(sam.name).amount, sam_adv_payment);
        BOOST_CHECK_EQUAL(v.get_last_cashback(alice.name).amount, pb(bs[0]) - alice_adv_payment);
        BOOST_CHECK_EQUAL(v.get_last_cashback(bob.name).amount, pb(bs[1]) - bob_adv_payment);
        BOOST_CHECK_EQUAL(v.get_last_cashback(sam.name).amount, pb(bs[2]) - sam_adv_payment);
    }
    {
        budget_payout_visitor v(db);
        create_budget(zorro, budget_type::post, 400, deadline);
        generate_block();

        auto bs = post_budget_service.get_budgets();
        BOOST_REQUIRE_EQUAL(bs.size(), 4u);
        auto alice_adv_payment = pb(bs[0]);
        auto bob_adv_payment = pb(bs[0]);
        auto sam_adv_payment = (75 * pb(bs[0]) + 10 * pb(bs[1])) / 85;
        auto zorro_adv_payment = (75 * pb(bs[0]) + 10 * pb(bs[1]) + 15 * pb(bs[2])) / 100;
        BOOST_CHECK_EQUAL(v.get_last_adv_payment(alice.name).amount, alice_adv_payment);
        BOOST_CHECK_EQUAL(v.get_last_adv_payment(bob.name).amount, bob_adv_payment);
        BOOST_CHECK_EQUAL(v.get_last_adv_payment(sam.name).amount, sam_adv_payment);
        BOOST_CHECK_EQUAL(v.get_last_adv_payment(zorro.name).amount, zorro_adv_payment);
        BOOST_CHECK_EQUAL(v.get_last_cashback(alice.name).amount, pb(bs[0]) - alice_adv_payment);
        BOOST_CHECK_EQUAL(v.get_last_cashback(bob.name).amount, pb(bs[1]) - bob_adv_payment);
        BOOST_CHECK_EQUAL(v.get_last_cashback(sam.name).amount, pb(bs[2]) - sam_adv_payment);
        BOOST_CHECK_EQUAL(v.get_last_cashback(zorro.name).amount, pb(bs[3]) - zorro_adv_payment);
    }
    {
        budget_payout_visitor v(db);
        create_budget(kenny, budget_type::post, 500, deadline);
        generate_block();

        auto bs = post_budget_service.get_budgets();
        BOOST_REQUIRE_EQUAL(bs.size(), 5u);
        auto alice_adv_payment = 0;
        auto bob_adv_payment = pb(bs[0]);
        auto sam_adv_payment = (65 * pb(bs[0]) + 10 * pb(bs[1])) / 75;
        auto zorro_adv_payment = (65 * pb(bs[0]) + 10 * pb(bs[1]) + 10 * pb(bs[2])) / 85;
        auto kenny_adv_payment = (65 * pb(bs[0]) + 10 * pb(bs[1]) + 10 * pb(bs[2]) + 15 * pb(bs[3])) / 100;
        BOOST_CHECK_EQUAL(v.get_last_adv_payment(alice.name).amount, alice_adv_payment);
        BOOST_CHECK_EQUAL(v.get_last_adv_payment(bob.name).amount, bob_adv_payment);
        BOOST_CHECK_EQUAL(v.get_last_adv_payment(sam.name).amount, sam_adv_payment);
        BOOST_CHECK_EQUAL(v.get_last_adv_payment(zorro.name).amount, zorro_adv_payment);
        BOOST_CHECK_EQUAL(v.get_last_adv_payment(kenny.name).amount, kenny_adv_payment);
        BOOST_CHECK_EQUAL(v.get_last_cashback(alice.name).amount, pb(bs[0]) - alice_adv_payment);
        BOOST_CHECK_EQUAL(v.get_last_cashback(bob.name).amount, pb(bs[1]) - bob_adv_payment);
        BOOST_CHECK_EQUAL(v.get_last_cashback(sam.name).amount, pb(bs[2]) - sam_adv_payment);
        BOOST_CHECK_EQUAL(v.get_last_cashback(zorro.name).amount, pb(bs[3]) - zorro_adv_payment);
        BOOST_CHECK_EQUAL(v.get_last_cashback(kenny.name).amount, pb(bs[4]) - kenny_adv_payment);
    }
    {
        budget_payout_visitor v(db);
        create_budget(cartman, budget_type::post, 600, deadline);
        generate_block();

        auto bs = post_budget_service.get_budgets();
        BOOST_REQUIRE_EQUAL(bs.size(), 6u);
        auto alice_adv_payment = 0;
        auto bob_adv_payment = 0;
        auto sam_adv_payment = pb(bs[1]);
        auto zorro_adv_payment = (65 * pb(bs[1]) + 10 * pb(bs[2])) / 75;
        auto kenny_adv_payment = (65 * pb(bs[1]) + 10 * pb(bs[2]) + 10 * pb(bs[3])) / 85;
        auto cartman_adv_payment = (65 * pb(bs[1]) + 10 * pb(bs[2]) + 10 * pb(bs[3]) + 15 * pb(bs[4])) / 100;
        BOOST_CHECK_EQUAL(v.get_last_adv_payment(alice.name).amount, alice_adv_payment);
        BOOST_CHECK_EQUAL(v.get_last_adv_payment(bob.name).amount, bob_adv_payment);
        BOOST_CHECK_EQUAL(v.get_last_adv_payment(sam.name).amount, sam_adv_payment);
        BOOST_CHECK_EQUAL(v.get_last_adv_payment(zorro.name).amount, zorro_adv_payment);
        BOOST_CHECK_EQUAL(v.get_last_adv_payment(kenny.name).amount, kenny_adv_payment);
        BOOST_CHECK_EQUAL(v.get_last_adv_payment(cartman.name).amount, cartman_adv_payment);
        BOOST_CHECK_EQUAL(v.get_last_cashback(alice.name).amount, pb(bs[0]) - alice_adv_payment);
        BOOST_CHECK_EQUAL(v.get_last_cashback(bob.name).amount, pb(bs[1]) - bob_adv_payment);
        BOOST_CHECK_EQUAL(v.get_last_cashback(sam.name).amount, pb(bs[2]) - sam_adv_payment);
        BOOST_CHECK_EQUAL(v.get_last_cashback(zorro.name).amount, pb(bs[3]) - zorro_adv_payment);
        BOOST_CHECK_EQUAL(v.get_last_cashback(kenny.name).amount, pb(bs[4]) - kenny_adv_payment);
        BOOST_CHECK_EQUAL(v.get_last_cashback(cartman.name).amount, pb(bs[5]) - cartman_adv_payment);
    }
}

SCORUM_TEST_CASE(adding_new_budgets_with_decreasing_per_block_test)
{
    /*
     * Coefficients: {100, 85, 75, 65}
     */

    // per block
    auto pb = [](const post_budget_object& b) { return b.per_block.amount.value; };

    int deadline = 10;

    {
        budget_payout_visitor v(db);
        create_budget(alice, budget_type::post, 600, deadline);
        generate_block();

        auto bs = post_budget_service.get_budgets();
        BOOST_REQUIRE_EQUAL(bs.size(), 1u);
        auto alice_adv_payment = pb(bs[0]);
        BOOST_CHECK_EQUAL(v.get_last_adv_payment(alice.name).amount, alice_adv_payment);
        BOOST_CHECK_EQUAL(v.get_last_cashback(alice.name).amount, pb(bs[0]) - alice_adv_payment);
    }
    {
        budget_payout_visitor v(db);
        create_budget(bob, budget_type::post, 500, deadline);
        generate_block();

        auto bs = post_budget_service.get_budgets();
        BOOST_REQUIRE_EQUAL(bs.size(), 2u);
        auto alice_adv_payment = pb(bs[1]);
        auto bob_adv_payment = pb(bs[1]);
        BOOST_CHECK_EQUAL(v.get_last_adv_payment(alice.name).amount, alice_adv_payment);
        BOOST_CHECK_EQUAL(v.get_last_adv_payment(bob.name).amount, bob_adv_payment);
        BOOST_CHECK_EQUAL(v.get_last_cashback(alice.name).amount, pb(bs[0]) - alice_adv_payment);
        BOOST_CHECK_EQUAL(v.get_last_cashback(bob.name).amount, pb(bs[1]) - bob_adv_payment);
    }
    {
        budget_payout_visitor v(db);
        create_budget(sam, budget_type::post, 400, deadline);
        generate_block();

        auto bs = post_budget_service.get_budgets();
        BOOST_REQUIRE_EQUAL(bs.size(), 3u);
        auto alice_adv_payment = (85 * pb(bs[2]) + 15 * pb(bs[1])) / 100;
        auto bob_adv_payment = pb(bs[2]);
        auto sam_adv_payment = pb(bs[2]);
        BOOST_CHECK_EQUAL(v.get_last_adv_payment(alice.name).amount, alice_adv_payment);
        BOOST_CHECK_EQUAL(v.get_last_adv_payment(bob.name).amount, bob_adv_payment);
        BOOST_CHECK_EQUAL(v.get_last_adv_payment(sam.name).amount, sam_adv_payment);
        BOOST_CHECK_EQUAL(v.get_last_cashback(alice.name).amount, pb(bs[0]) - alice_adv_payment);
        BOOST_CHECK_EQUAL(v.get_last_cashback(bob.name).amount, pb(bs[1]) - bob_adv_payment);
        BOOST_CHECK_EQUAL(v.get_last_cashback(sam.name).amount, pb(bs[2]) - sam_adv_payment);
    }
    {
        budget_payout_visitor v(db);
        create_budget(zorro, budget_type::post, 300, deadline);
        generate_block();

        auto bs = post_budget_service.get_budgets();
        BOOST_REQUIRE_EQUAL(bs.size(), 4u);
        auto alice_adv_payment = (75 * pb(bs[3]) + 10 * pb(bs[2]) + 15 * pb(bs[1])) / 100;
        auto bob_adv_payment = (75 * pb(bs[3]) + 10 * pb(bs[2])) / 85;
        auto sam_adv_payment = pb(bs[3]);
        auto zorro_adv_payment = pb(bs[3]);
        BOOST_CHECK_EQUAL(v.get_last_adv_payment(alice.name).amount, alice_adv_payment);
        BOOST_CHECK_EQUAL(v.get_last_adv_payment(bob.name).amount, bob_adv_payment);
        BOOST_CHECK_EQUAL(v.get_last_adv_payment(sam.name).amount, sam_adv_payment);
        BOOST_CHECK_EQUAL(v.get_last_adv_payment(zorro.name).amount, zorro_adv_payment);
        BOOST_CHECK_EQUAL(v.get_last_cashback(alice.name).amount, pb(bs[0]) - alice_adv_payment);
        BOOST_CHECK_EQUAL(v.get_last_cashback(bob.name).amount, pb(bs[1]) - bob_adv_payment);
        BOOST_CHECK_EQUAL(v.get_last_cashback(sam.name).amount, pb(bs[2]) - sam_adv_payment);
        BOOST_CHECK_EQUAL(v.get_last_cashback(zorro.name).amount, pb(bs[3]) - zorro_adv_payment);
    }
    {
        budget_payout_visitor v(db);
        create_budget(kenny, budget_type::post, 200, deadline);
        generate_block();

        auto bs = post_budget_service.get_budgets();
        BOOST_REQUIRE_EQUAL(bs.size(), 5u);
        auto alice_adv_payment = (65 * pb(bs[4]) + 10 * pb(bs[3]) + 10 * pb(bs[2]) + 15 * pb(bs[1])) / 100;
        auto bob_adv_payment = (65 * pb(bs[4]) + 10 * pb(bs[3]) + 10 * pb(bs[2])) / 85;
        auto sam_adv_payment = (65 * pb(bs[4]) + 10 * pb(bs[3])) / 75;
        auto zorro_adv_payment = pb(bs[4]);
        auto kenny_adv_payment = 0;
        BOOST_CHECK_EQUAL(v.get_last_adv_payment(alice.name).amount, alice_adv_payment);
        BOOST_CHECK_EQUAL(v.get_last_adv_payment(bob.name).amount, bob_adv_payment);
        BOOST_CHECK_EQUAL(v.get_last_adv_payment(sam.name).amount, sam_adv_payment);
        BOOST_CHECK_EQUAL(v.get_last_adv_payment(zorro.name).amount, zorro_adv_payment);
        BOOST_CHECK_EQUAL(v.get_last_adv_payment(kenny.name).amount, kenny_adv_payment);
        BOOST_CHECK_EQUAL(v.get_last_cashback(alice.name).amount, pb(bs[0]) - alice_adv_payment);
        BOOST_CHECK_EQUAL(v.get_last_cashback(bob.name).amount, pb(bs[1]) - bob_adv_payment);
        BOOST_CHECK_EQUAL(v.get_last_cashback(sam.name).amount, pb(bs[2]) - sam_adv_payment);
        BOOST_CHECK_EQUAL(v.get_last_cashback(zorro.name).amount, pb(bs[3]) - zorro_adv_payment);
        BOOST_CHECK_EQUAL(v.get_last_cashback(kenny.name).amount, pb(bs[4]) - kenny_adv_payment);
    }
    {
        budget_payout_visitor v(db);
        create_budget(cartman, budget_type::post, 100, deadline);
        generate_block();

        auto bs = post_budget_service.get_budgets();
        BOOST_REQUIRE_EQUAL(bs.size(), 6u);
        auto alice_adv_payment = (65 * pb(bs[4]) + 10 * pb(bs[3]) + 10 * pb(bs[2]) + 15 * pb(bs[1])) / 100;
        auto bob_adv_payment = (65 * pb(bs[4]) + 10 * pb(bs[3]) + 10 * pb(bs[2])) / 85;
        auto sam_adv_payment = (65 * pb(bs[4]) + 10 * pb(bs[3])) / 75;
        auto zorro_adv_payment = pb(bs[4]);
        auto kenny_adv_payment = 0;
        auto cartman_adv_payment = 0;
        BOOST_CHECK_EQUAL(v.get_last_adv_payment(alice.name).amount, alice_adv_payment);
        BOOST_CHECK_EQUAL(v.get_last_adv_payment(bob.name).amount, bob_adv_payment);
        BOOST_CHECK_EQUAL(v.get_last_adv_payment(sam.name).amount, sam_adv_payment);
        BOOST_CHECK_EQUAL(v.get_last_adv_payment(zorro.name).amount, zorro_adv_payment);
        BOOST_CHECK_EQUAL(v.get_last_adv_payment(kenny.name).amount, kenny_adv_payment);
        BOOST_CHECK_EQUAL(v.get_last_adv_payment(cartman.name).amount, cartman_adv_payment);
        BOOST_CHECK_EQUAL(v.get_last_cashback(alice.name).amount, pb(bs[0]) - alice_adv_payment);
        BOOST_CHECK_EQUAL(v.get_last_cashback(bob.name).amount, pb(bs[1]) - bob_adv_payment);
        BOOST_CHECK_EQUAL(v.get_last_cashback(sam.name).amount, pb(bs[2]) - sam_adv_payment);
        BOOST_CHECK_EQUAL(v.get_last_cashback(zorro.name).amount, pb(bs[3]) - zorro_adv_payment);
        BOOST_CHECK_EQUAL(v.get_last_cashback(kenny.name).amount, pb(bs[4]) - kenny_adv_payment);
        BOOST_CHECK_EQUAL(v.get_last_cashback(cartman.name).amount, pb(bs[5]) - cartman_adv_payment);
    }
}

SCORUM_TEST_CASE(insert_new_budget_in_the_middle_test)
{
    // per block
    auto pb = [](const banner_budget_object& b) { return b.per_block.amount.value; };

    int deadline = 10;

    create_budget(alice, budget_type::banner, 100, deadline);
    create_budget(bob, budget_type::banner, 200, deadline);
    create_budget(zorro, budget_type::banner, 400, deadline);
    create_budget(kenny, budget_type::banner, 500, deadline);
    create_budget(cartman, budget_type::banner, 600, deadline);

    {
        budget_payout_visitor v(db);
        generate_block();

        auto bs = banner_budget_service.get_budgets();
        BOOST_REQUIRE_EQUAL(bs.size(), 5u);
        auto alice_adv_payment = 0;
        auto bob_adv_payment = pb(bs[0]);
        auto zorro_adv_payment = (65 * pb(bs[0]) + 10 * pb(bs[1])) / 75;
        auto kenny_adv_payment = (65 * pb(bs[0]) + 10 * pb(bs[1]) + 10 * pb(bs[2])) / 85;
        auto cartman_adv_payment = (65 * pb(bs[0]) + 10 * pb(bs[1]) + 10 * pb(bs[2]) + 15 * pb(bs[3])) / 100;
        BOOST_CHECK_EQUAL(v.get_last_adv_payment(alice.name).amount, alice_adv_payment);
        BOOST_CHECK_EQUAL(v.get_last_adv_payment(bob.name).amount, bob_adv_payment);
        BOOST_CHECK_EQUAL(v.get_last_adv_payment(zorro.name).amount, zorro_adv_payment);
        BOOST_CHECK_EQUAL(v.get_last_adv_payment(kenny.name).amount, kenny_adv_payment);
        BOOST_CHECK_EQUAL(v.get_last_adv_payment(cartman.name).amount, cartman_adv_payment);
        BOOST_CHECK_EQUAL(v.get_last_cashback(alice.name).amount, pb(bs[0]) - alice_adv_payment);
        BOOST_CHECK_EQUAL(v.get_last_cashback(bob.name).amount, pb(bs[1]) - bob_adv_payment);
        BOOST_CHECK_EQUAL(v.get_last_cashback(zorro.name).amount, pb(bs[2]) - zorro_adv_payment);
        BOOST_CHECK_EQUAL(v.get_last_cashback(kenny.name).amount, pb(bs[3]) - kenny_adv_payment);
        BOOST_CHECK_EQUAL(v.get_last_cashback(cartman.name).amount, pb(bs[4]) - cartman_adv_payment);
    }
    {
        budget_payout_visitor v(db);
        create_budget(sam, budget_type::banner, 300, deadline);
        generate_block();

        // NOTE: the order of budgets: alice-bob-zorro-kenny-cartman-sam
        auto bs = banner_budget_service.get_budgets();
        BOOST_REQUIRE_EQUAL(bs.size(), 6u);
        auto alice_adv_payment = 0;
        auto bob_adv_payment = 0;
        auto sam_adv_payment = pb(bs[1]);
        auto zorro_adv_payment = (65 * pb(bs[1]) + 10 * pb(bs[5])) / 75;
        auto kenny_adv_payment = (65 * pb(bs[1]) + 10 * pb(bs[5]) + 10 * pb(bs[2])) / 85;
        auto cartman_adv_payment = (65 * pb(bs[1]) + 10 * pb(bs[5]) + 10 * pb(bs[2]) + 15 * pb(bs[3])) / 100;
        BOOST_CHECK_EQUAL(v.get_last_adv_payment(alice.name).amount, alice_adv_payment);
        BOOST_CHECK_EQUAL(v.get_last_adv_payment(bob.name).amount, bob_adv_payment);
        BOOST_CHECK_EQUAL(v.get_last_adv_payment(sam.name).amount, sam_adv_payment);
        BOOST_CHECK_EQUAL(v.get_last_adv_payment(zorro.name).amount, zorro_adv_payment);
        BOOST_CHECK_EQUAL(v.get_last_adv_payment(kenny.name).amount, kenny_adv_payment);
        BOOST_CHECK_EQUAL(v.get_last_adv_payment(cartman.name).amount, cartman_adv_payment);
        BOOST_CHECK_EQUAL(v.get_last_cashback(alice.name).amount, pb(bs[0]) - alice_adv_payment);
        BOOST_CHECK_EQUAL(v.get_last_cashback(bob.name).amount, pb(bs[1]) - bob_adv_payment);
        BOOST_CHECK_EQUAL(v.get_last_cashback(sam.name).amount, pb(bs[5]) - sam_adv_payment);
        BOOST_CHECK_EQUAL(v.get_last_cashback(zorro.name).amount, pb(bs[2]) - zorro_adv_payment);
        BOOST_CHECK_EQUAL(v.get_last_cashback(kenny.name).amount, pb(bs[3]) - kenny_adv_payment);
        BOOST_CHECK_EQUAL(v.get_last_cashback(cartman.name).amount, pb(bs[4]) - cartman_adv_payment);
    }
}

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

SCORUM_TEST_CASE(two_post_budget_from_same_acc_vcg_algorithm_test)
{
    /*
     * VCG algorithm for 3 bets/2 coefficients:
     *
     * CPB3=Bid3 -- this one is not handled by VCG algorithm. So we decided that CPB3 will be equal to CPB2
     * CPB2=Bid3
     * CPB1=(X2*Bid3+(X1-X2)*Bid2)/X1
     * -------------------------------------------------------
     *
     * VCG algorithm for 2 bets/1 coefficients [current test case]:
     * CPB2=Bid2 -- this one is not handled by VCG algorithm. So we decided that CPB2 will be equal to CPB1
     * CPB1=Bid2
     *
     * NOTE: winner's per-block payment equals looser's bet in this case.
     */
    create_budget(alice, budget_type::post, 100, 10);
    generate_block();

    {
        auto budgets = post_budget_service.get_budgets();
        BOOST_REQUIRE_EQUAL(budgets.size(), 1u);
        // There is a single budget so whole 'per-block' amount should be decreased from budget
        BOOST_CHECK_EQUAL(budgets[0].get().balance.amount, 100 - budgets[0].get().per_block.amount);
    }

    auto alice_balance_before = account_service.get_account(alice.name).balance;
    create_budget(alice, budget_type::post, 200, 10);
    generate_block();
    auto alice_balance_after = account_service.get_account(alice.name).balance;

    {
        auto budgets = post_budget_service.get_budgets();
        BOOST_REQUIRE_EQUAL(budgets.size(), 2u);
        // 'b1' is a loser
        BOOST_CHECK_EQUAL(budgets[0].get().balance.amount, 100 - 2 * budgets[0].get().per_block.amount);
        // There are two budgets. 'b2' is a winner. According to VCG algorithm its payment should be equal to
        // looser's per-block payment. The rest should be returned to account.
        BOOST_CHECK_EQUAL(budgets[1].get().balance.amount, 200 - budgets[1].get().per_block.amount);
        auto returned_to_acc = budgets[1].get().per_block.amount - budgets[0].get().per_block.amount;
        BOOST_CHECK_EQUAL(alice_balance_before.amount - 200 + returned_to_acc, alice_balance_after.amount);
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
