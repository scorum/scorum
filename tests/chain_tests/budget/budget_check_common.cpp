#include <boost/test/unit_test.hpp>

#include "budget_check_common.hpp"

namespace budget_check_common {

budget_check_fixture::budget_check_fixture()
    : post_budget_service(db.post_budget_service())
    , banner_budget_service(db.banner_budget_service())
    , creator(db)
{
}

std::string budget_check_fixture::get_unique_permlink()
{
    static uint32_t permlink_no = 0;
    permlink_no++;

    return boost::lexical_cast<std::string>(permlink_no);
}

void budget_check_fixture::create_budget(const Actor& owner, const budget_type type)
{
    create_budget(owner, type, BUDGET_BALANCE_DEFAULT, BUDGET_DEADLINE_IN_BLOCKS_DEFAULT);
}

void budget_check_fixture::create_budget(const Actor& owner,
                                         const budget_type type,
                                         int balance,
                                         int deadline_in_blocks)
{
    create_budget(owner, type, balance, 0, deadline_in_blocks);
}

void budget_check_fixture::create_budget(
    const Actor& owner, const budget_type type, int balance, int start_in_blocks, int deadline_in_blocks)
{
    create_budget_operation op;
    op.owner = owner.name;
    op.type = type;
    op.balance = asset(balance, SCORUM_SYMBOL);
    if (start_in_blocks > 0)
    {
        op.start = db.get_slot_time(start_in_blocks);
    }
    op.deadline = db.get_slot_time(deadline_in_blocks);
    op.content_permlink = get_unique_permlink();

    push_operation_only(op, owner.private_key);
}

void budget_check_fixture::create_budget(const Actor& owner,
                                         const budget_type type,
                                         const asset& balance,
                                         const fc::time_point_sec& start,
                                         const fc::time_point_sec& deadline)
{
    create_budget_operation op;
    op.owner = owner.name;
    op.type = type;
    op.balance = balance;
    op.start = start;
    op.deadline = deadline;
    op.content_permlink = get_unique_permlink();

    push_operation_only(op, owner.private_key);

    generate_block();
}

} // namespace database_fixture
