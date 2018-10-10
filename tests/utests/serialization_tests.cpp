/*
 * Copyright (c) 2015 Cryptonomex, Inc., and contributors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include <boost/test/unit_test.hpp>

#include <scorum/protocol/asset.hpp>
#include <scorum/protocol/version.hpp>

#include <scorum/protocol/transaction.hpp>

#include <boost/uuid/uuid_generators.hpp>
#include <fc/io/json.hpp>

#include "utils.hpp"
#include "defines.hpp"

using scorum::protocol::asset;
using scorum::protocol::version;
using scorum::protocol::extended_private_key_type;
using scorum::protocol::extended_public_key_type;
using scorum::protocol::hardfork_version;

using scorum::protocol::create_budget_operation;

BOOST_AUTO_TEST_SUITE(serialization_tests)

BOOST_AUTO_TEST_CASE(account_name_type_test)
{
    auto test = [](const std::string& data) {
        fc::fixed_string<> a(data);
        std::string b(data);

        auto ap = fc::raw::pack(a);
        auto bp = fc::raw::pack(b);

        BOOST_CHECK_EQUAL(ap.size(), bp.size());
        BOOST_CHECK(std::equal(ap.begin(), ap.end(), bp.begin()));

        auto au = fc::raw::unpack<std::string>(ap);
        auto bu = fc::raw::unpack<fc::fixed_string<>>(bp);

        BOOST_CHECK_EQUAL(au, bu);
    };

    test(std::string());
    test("helloworld");
    test("1234567890123456");
}

SCORUM_TEST_CASE(asset_test)
{
    BOOST_CHECK_EQUAL(asset(0, SCORUM_SYMBOL).decimals(), SCORUM_CURRENCY_PRECISION);
    BOOST_CHECK_EQUAL(asset(0, SCORUM_SYMBOL).symbol_name(), "SCR");
    BOOST_CHECK_EQUAL(asset(0, SCORUM_SYMBOL).to_string(), "0.000000000 SCR");

    BOOST_CHECK_THROW(asset::from_string("1.0000000000000000000000 SCR"), fc::exception);
    BOOST_CHECK_THROW(asset::from_string("1.000000000SCR"), fc::exception);
    BOOST_CHECK_THROW(asset::from_string("1. 333333333 SCR"),
                      fc::exception); // Fails because symbol is '333333333 SCR', which is too long
    BOOST_CHECK_THROW(asset::from_string("1 .333333333 SCR"), fc::exception);
    BOOST_CHECK_THROW(asset::from_string("1 .333333333 X"), fc::exception);
    BOOST_CHECK_THROW(asset::from_string("1 .333333333"), fc::exception);
    BOOST_CHECK_THROW(asset::from_string("1 1.1"), fc::exception);
    BOOST_CHECK_THROW(asset::from_string("11111111111111111111111111111111111111111111111 SCR"), fc::exception);
    BOOST_CHECK_THROW(asset::from_string("1.1.1 SCR"), fc::exception);
    BOOST_CHECK_THROW(asset::from_string("1.abc SCR"), fc::exception);
    BOOST_CHECK_THROW(asset::from_string(" SCR"), fc::exception);
    BOOST_CHECK_THROW(asset::from_string("SCR"), fc::exception);
    BOOST_CHECK_THROW(asset::from_string("1.333333333"), fc::exception);
    BOOST_CHECK_THROW(asset::from_string("1.333333333 "), fc::exception);
    BOOST_CHECK_THROW(asset::from_string(""), fc::exception);
    BOOST_CHECK_THROW(asset::from_string(" "), fc::exception);
    BOOST_CHECK_THROW(asset::from_string("  "), fc::exception);
    BOOST_CHECK_THROW(asset::from_string("1.000111222 LARGENAME"), fc::exception);
    BOOST_CHECK_THROW(asset::from_string("1.000111222 WRONG"), fc::exception);
    BOOST_CHECK_THROW(asset::from_string("1. SP"), fc::exception);

    BOOST_CHECK_EQUAL(asset::from_string("100.012000000 SCR").amount.value, 100012000000);
    BOOST_CHECK_EQUAL(asset::from_string("0.000000120 SCR").amount.value, 120);
    BOOST_CHECK_EQUAL(asset::from_string("100.000000000 SCR").amount.value, 100000000000);
    BOOST_CHECK_EQUAL(asset::from_string("100.012000000 SP").amount.value, 100012000000);
    BOOST_CHECK_EQUAL(asset::from_string("0.000000120 SP").amount.value, 120);
    BOOST_CHECK_EQUAL(asset::from_string("100.000000000 SP").amount.value, 100000000000);

    asset scorum = asset::from_string("1.123456000 SCR");
    asset tmp = asset::from_string("0.000000456 SCR");
    BOOST_CHECK_EQUAL(tmp.amount.value, 456);
    tmp = asset::from_string("0.000000056 SCR");
    BOOST_CHECK_EQUAL(tmp.amount.value, 56);

    BOOST_CHECK(std::abs(scorum.to_real() - 1.123456) < 0.0005);
    BOOST_CHECK_EQUAL(scorum.amount.value, 1123456000);
    BOOST_CHECK_EQUAL(scorum.decimals(), SCORUM_CURRENCY_PRECISION);
    BOOST_CHECK_EQUAL(scorum.symbol_name(), "SCR");
    BOOST_CHECK_EQUAL(scorum.to_string(), "1.123456000 SCR");
    BOOST_CHECK_EQUAL(scorum.symbol(), SCORUM_SYMBOL);
    BOOST_CHECK_EQUAL(asset(50, SCORUM_SYMBOL).to_string(), "0.000000050 SCR");
    BOOST_CHECK_EQUAL(asset(5e+10, SCORUM_SYMBOL).to_string(), "50.000000000 SCR");
}

SCORUM_TEST_CASE(json_tests)
{
    auto var = fc::json::variants_from_string("10.6 ");
    var = fc::json::variants_from_string("10.5");
}

SCORUM_TEST_CASE(extended_private_key_type_test)
{
    fc::ecc::extended_private_key key
        = fc::ecc::extended_private_key(fc::ecc::private_key::generate(), fc::sha256(), 0, 0, 0);
    extended_private_key_type type = extended_private_key_type(key);
    std::string packed = std::string(type);
    extended_private_key_type unpacked = extended_private_key_type(packed);
    BOOST_CHECK(type == unpacked);
}

SCORUM_TEST_CASE(extended_public_key_type_test)
{
    fc::ecc::extended_public_key key
        = fc::ecc::extended_public_key(fc::ecc::private_key::generate().get_public_key(), fc::sha256(), 0, 0, 0);
    extended_public_key_type type = extended_public_key_type(key);
    std::string packed = std::string(type);
    extended_public_key_type unpacked = extended_public_key_type(packed);
    BOOST_CHECK(type == unpacked);
}

SCORUM_TEST_CASE(version_test)
{
    BOOST_REQUIRE_EQUAL(std::string(version(1, 2, 3)), "1.2.3");

    fc::variant ver_str("3.0.0");
    version ver;
    fc::from_variant(ver_str, ver);
    BOOST_REQUIRE(ver == version(3, 0, 0));

    ver_str = fc::variant("0.0.0");
    fc::from_variant(ver_str, ver);
    BOOST_REQUIRE(ver == version());

    ver_str = fc::variant("1.0.1");
    fc::from_variant(ver_str, ver);
    BOOST_REQUIRE(ver == version(1, 0, 1));

    ver_str = fc::variant("1_0_1");
    fc::from_variant(ver_str, ver);
    BOOST_REQUIRE(ver == version(1, 0, 1));

    ver_str = fc::variant("12.34.56");
    fc::from_variant(ver_str, ver);
    BOOST_REQUIRE(ver == version(12, 34, 56));

    ver_str = fc::variant("256.0.0");
    SCORUM_REQUIRE_THROW(fc::from_variant(ver_str, ver), fc::exception);

    ver_str = fc::variant("0.256.0");
    SCORUM_REQUIRE_THROW(fc::from_variant(ver_str, ver), fc::exception);

    ver_str = fc::variant("0.0.65536");
    SCORUM_REQUIRE_THROW(fc::from_variant(ver_str, ver), fc::exception);

    ver_str = fc::variant("1.0");
    SCORUM_REQUIRE_THROW(fc::from_variant(ver_str, ver), fc::exception);

    ver_str = fc::variant("1.0.0.1");
    SCORUM_REQUIRE_THROW(fc::from_variant(ver_str, ver), fc::exception);
}

SCORUM_TEST_CASE(hardfork_version_test)
{
    BOOST_REQUIRE_EQUAL(std::string(hardfork_version(1, 2)), "1.2.0");

    fc::variant ver_str("3.0.0");
    hardfork_version ver;
    fc::from_variant(ver_str, ver);
    BOOST_REQUIRE(ver == hardfork_version(3, 0));

    ver_str = fc::variant("0.0.0");
    fc::from_variant(ver_str, ver);
    BOOST_REQUIRE(ver == hardfork_version());

    ver_str = fc::variant("1.0.0");
    fc::from_variant(ver_str, ver);
    BOOST_REQUIRE(ver == hardfork_version(1, 0));

    ver_str = fc::variant("1_0_0");
    fc::from_variant(ver_str, ver);
    BOOST_REQUIRE(ver == hardfork_version(1, 0));

    ver_str = fc::variant("12.34.00");
    fc::from_variant(ver_str, ver);
    BOOST_REQUIRE(ver == hardfork_version(12, 34));

    ver_str = fc::variant("256.0.0");
    SCORUM_REQUIRE_THROW(fc::from_variant(ver_str, ver), fc::exception);

    ver_str = fc::variant("0.256.0");
    SCORUM_REQUIRE_THROW(fc::from_variant(ver_str, ver), fc::exception);

    ver_str = fc::variant("0.0.1");
    fc::from_variant(ver_str, ver);
    BOOST_REQUIRE(ver == hardfork_version(0, 0));

    ver_str = fc::variant("1.0");
    SCORUM_REQUIRE_THROW(fc::from_variant(ver_str, ver), fc::exception);

    ver_str = fc::variant("1.0.0.1");
    SCORUM_REQUIRE_THROW(fc::from_variant(ver_str, ver), fc::exception);
}

SCORUM_TEST_CASE(serialize_create_budget_operation_to_binary_test)
{
    create_budget_operation op;
    op.uuid = boost::uuids::string_generator()("6DCD3132-E5DF-480A-89A8-91984BCA0A09");
    op.type = scorum::protocol::budget_type::post;
    op.owner = "initdelegate";
    op.balance = ASSET("10.000000000 SCR");
    op.start = fc::time_point_sec::from_iso_string("2018-08-03T10:12:43");
    op.deadline = fc::time_point_sec::from_iso_string("2018-08-03T10:13:13");
    op.json_metadata = "{}";

    BOOST_CHECK_EQUAL("00000000000000006dcd3132e5df480a89a891984bca0a090c696e697464656c6567617465027b7d00e40b5402000000"
                      "09534352000000009b2a645bb92a645b",
                      utils::to_hex(op));
}

BOOST_AUTO_TEST_SUITE_END()
