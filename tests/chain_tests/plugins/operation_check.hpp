#pragma once

#include <boost/test/unit_test.hpp>

#include <scorum/protocol/operations.hpp>

#include <string>

using namespace scorum;
using namespace scorum::chain;
using namespace scorum::protocol;
using namespace scorum::app;

namespace operation_tests {

struct check_saved_opetations_visitor
{
    check_saved_opetations_visitor(const operation& input_op)
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
}
