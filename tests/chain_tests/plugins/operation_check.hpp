#pragma once

#include <boost/test/unit_test.hpp>

#include <scorum/protocol/operations.hpp>

#include <string>
#include <fc/log/logger.hpp>

#include <vector>
#include <algorithm>

using namespace scorum;
using namespace scorum::chain;
using namespace scorum::protocol;
using namespace scorum::app;

namespace operation_tests {

struct check_opetation_visitor
{
    check_opetation_visitor(const operation& input_op)
        : _input_op(input_op)
    {
    }

    using result_type = void;

    void operator()(const transfer_to_scorumpower_operation& saved_op) const
    {
        BOOST_REQUIRE_EQUAL(_input_op.which(), operation(saved_op).which());
        const auto& input_op = _input_op.get<transfer_to_scorumpower_operation>();

        BOOST_REQUIRE_EQUAL(saved_op.amount, input_op.amount);
        BOOST_REQUIRE_EQUAL(saved_op.from, input_op.from);
        BOOST_REQUIRE_EQUAL(saved_op.to, input_op.to);
    }

    void operator()(const transfer_operation& saved_op) const
    {
        BOOST_REQUIRE_EQUAL(_input_op.which(), operation(saved_op).which());
        const auto& input_op = _input_op.get<transfer_operation>();

        BOOST_REQUIRE_EQUAL(saved_op.amount, input_op.amount);
        BOOST_REQUIRE_EQUAL(saved_op.from, input_op.from);
        BOOST_REQUIRE_EQUAL(saved_op.to, input_op.to);
        BOOST_REQUIRE_EQUAL(saved_op.memo, input_op.memo);
    }

    void operator()(const witness_update_operation& saved_op) const
    {
        BOOST_REQUIRE_EQUAL(_input_op.which(), operation(saved_op).which());
        const auto& input_op = _input_op.get<witness_update_operation>();

        BOOST_REQUIRE_EQUAL(saved_op.owner, input_op.owner);
        BOOST_REQUIRE_EQUAL(saved_op.url, input_op.url);
        BOOST_REQUIRE_EQUAL((std::string)saved_op.block_signing_key, (std::string)input_op.block_signing_key);
    }

    template <typename Op> void operator()(const Op&) const
    {
        // invalid type recived
        BOOST_REQUIRE(false);
    }

private:
    const operation& _input_op;
};

using opetations_type = std::vector<operation>;

struct check_opetations_list_visitor
{
    check_opetations_list_visitor(const opetations_type& input_ops)
        : _input_ops(input_ops)
    {
        FC_ASSERT(!_input_ops.empty());
    }

    using result_type = void;

    template <typename Op> void operator()(const Op& op) const
    {
        auto it = std::find(std::begin(_input_ops), std::end(_input_ops), op);
        if (it != _input_ops.end())
        {
            check_opetation_visitor check(*it);
            check(op);
            _input_ops.erase(it);
        }
    }

    bool successed() const
    {
        return _input_ops.empty();
    }

private:
    mutable opetations_type _input_ops;
};

struct view_opetation_visitor
{
    using result_type = void;

    template <typename Op> void operator()(const Op& op) const
    {
        fc::variant vo;
        fc::to_variant(op, vo);

        ilog("${j}", ("j", fc::json::to_pretty_string(vo)));
    }
};
}
