#pragma once

#include <boost/version.hpp>
#include <scorum/protocol/config.hpp>

#include <fc/io/json.hpp>

#define TEST_CHAIN_ID fc::sha256::hash("testnet")

#define TEST_SHARED_MEM_SIZE_10MB (1024 * 1024 * 10)

#define TEST_ACCOUNTS_INITIAL_SUPPLY asset(10000000000ll, SCORUM_SYMBOL)
#define TEST_REWARD_INITIAL_SUPPLY                                                                                     \
    asset(SCORUM_REWARDS_INITIAL_SUPPLY_PERIOD_IN_DAYS* SCORUM_BLOCKS_PER_DAY * 100, SCORUM_SYMBOL)
#define TEST_GENESIS_TIMESTAMP (scorum::protocol::detail::get_config().initial_date.sec_since_epoch())

#define TEST_INIT_DELEGATE_NAME "initdelegate"
#define TEST_INIT_KEY TEST_INIT_DELEGATE_NAME
#define TEST_NUM_INIT_DELEGATES 1

#define SCORUM_MIN_PRODUCER_REWARD asset(1000, SCORUM_SYMBOL)

template <class T> T make_test_index_object()
{
    struct constructor
    {
        void operator()(const T&)
        {
        }
    };

    return std::move(T(constructor(), std::allocator<T>()));
}

#define INIT_MEMBER_OBJ(obj) obj(make_test_index_object<decltype(obj)>())
#define INIT_OBJ(type) make_test_index_object<type>()

// See below
#define REQUIRE_OP_VALIDATION_SUCCESS(op, field, value)                                                                \
    {                                                                                                                  \
        const auto temp = op.field;                                                                                    \
        op.field = value;                                                                                              \
        op.validate();                                                                                                 \
        op.field = temp;                                                                                               \
    }
#define REQUIRE_OP_EVALUATION_SUCCESS(op, field, value)                                                                \
    {                                                                                                                  \
        const auto temp = op.field;                                                                                    \
        op.field = value;                                                                                              \
        trx.operations.back() = op;                                                                                    \
        op.field = temp;                                                                                               \
        db.push_transaction(trx, ~0);                                                                                  \
    }

/*#define SCORUM_REQUIRE_THROW( expr, exc_type )          \
{                                                         \
   std::string req_throw_info = fc::json::to_string(      \
      fc::mutable_variant_object()                        \
      ("source_file", __FILE__)                           \
      ("source_lineno", __LINE__)                         \
      ("expr", #expr)                                     \
      ("exc_type", #exc_type)                             \
      );                                                  \
   if( fc::enable_record_assert_trip )                    \
      std::cout << "SCORUM_REQUIRE_THROW begin "        \
         << req_throw_info << std::endl;                  \
   BOOST_REQUIRE_THROW( expr, exc_type );                 \
   if( fc::enable_record_assert_trip )                    \
      std::cout << "SCORUM_REQUIRE_THROW end "          \
         << req_throw_info << std::endl;                  \
}*/

#define SCORUM_CHECK_EXCEPTION(test_code, exception, message)                                                          \
    try                                                                                                                \
    {                                                                                                                  \
        test_code;                                                                                                     \
        BOOST_FAIL("exception wasn't thrown out.");                                                                    \
    }                                                                                                                  \
    catch (exception & e)                                                                                              \
    {                                                                                                                  \
        BOOST_REQUIRE(e.get_log().size() == 1);                                                                        \
        bool check_exception_message = e.get_log().front().get_message().find(message) != std::string::npos;           \
        if (!check_exception_message)                                                                                  \
            std::cout << "EXCEPTION:" << e.get_log().front().get_message() << std::endl;                               \
        BOOST_CHECK(check_exception_message);                                                                          \
    }

#define SCORUM_REQUIRE_THROW(expr, exc_type) BOOST_REQUIRE_THROW(expr, exc_type);

#define SCORUM_CHECK_THROW(expr, exc_type)                                                                             \
    {                                                                                                                  \
        std::string req_throw_info = fc::json::to_string(fc::mutable_variant_object()("source_file", __FILE__)(        \
            "source_lineno", __LINE__)("expr", #expr)("exc_type", #exc_type));                                         \
        if (fc::enable_record_assert_trip)                                                                             \
            std::cout << "SCORUM_CHECK_THROW begin " << req_throw_info << std::endl;                                   \
        BOOST_CHECK_THROW(expr, exc_type);                                                                             \
        if (fc::enable_record_assert_trip)                                                                             \
            std::cout << "SCORUM_CHECK_THROW end " << req_throw_info << std::endl;                                     \
    }

#define REQUIRE_OP_VALIDATION_FAILURE_2(op, field, value, exc_type)                                                    \
    {                                                                                                                  \
        const auto temp = op.field;                                                                                    \
        op.field = value;                                                                                              \
        SCORUM_REQUIRE_THROW(op.validate(), exc_type);                                                                 \
        op.field = temp;                                                                                               \
    }
#define REQUIRE_OP_VALIDATION_FAILURE(op, field, value) REQUIRE_OP_VALIDATION_FAILURE_2(op, field, value, fc::exception)

#define REQUIRE_THROW_WITH_VALUE_2(op, field, value, exc_type)                                                         \
    {                                                                                                                  \
        auto bak = op.field;                                                                                           \
        op.field = value;                                                                                              \
        trx.operations.back() = op;                                                                                    \
        op.field = bak;                                                                                                \
        SCORUM_REQUIRE_THROW(db.push_transaction(trx, ~0), exc_type);                                                  \
    }

#define REQUIRE_THROW_WITH_VALUE(op, field, value) REQUIRE_THROW_WITH_VALUE_2(op, field, value, fc::exception)

/// This simply resets v back to its default-constructed value. Requires v to have a working assignment operator and
/// default constructor.
#define RESET(v) v = decltype(v)()
/// This allows me to build consecutive test cases. It's pretty ugly, but it works well enough for unit tests.
/// i.e. This allows a test on update_account to begin with the database at the end state of create_account.
#define INVOKE(test)                                                                                                   \
    ((struct test*)this)->test_method();                                                                               \
    trx.clear()

#define PREP_ACTOR(name)                                                                                               \
    fc::ecc::private_key name##_private_key = generate_private_key(BOOST_PP_STRINGIZE(name));                          \
    fc::ecc::private_key name##_post_key = generate_private_key(std::string(BOOST_PP_STRINGIZE(name)) + "_post");      \
    public_key_type name##_public_key = name##_private_key.get_public_key();

#define ACTOR(name)                                                                                                    \
    PREP_ACTOR(name)                                                                                                   \
    const auto& name = account_create(BOOST_PP_STRINGIZE(name), name##_public_key, name##_post_key.get_public_key());  \
    account_id_type name##_id = name.id;                                                                               \
    (void)name##_id;

#define GET_ACTOR(name)                                                                                                \
    fc::ecc::private_key name##_private_key = generate_private_key(BOOST_PP_STRINGIZE(name));                          \
    const account_object& name = get_account(BOOST_PP_STRINGIZE(name));                                                \
    account_id_type name##_id = name.id;                                                                               \
    (void)name##_id

#define ACTORS_IMPL(r, data, elem) ACTOR(elem)
#define ACTORS(names)                                                                                                  \
    BOOST_PP_SEQ_FOR_EACH(ACTORS_IMPL, ~, names)                                                                       \
    validate_database();

#define ASSET(s) asset::from_string(s)
#define ASSET_NULL_SCR asset(0, SCORUM_SYMBOL)
#define ASSET_NULL_SP asset(0, SP_SYMBOL)

#define ASSET_SCR(s) asset((scorum::protocol::share_value_type)s, SCORUM_SYMBOL)
#define ASSET_SP(s) asset((scorum::protocol::share_value_type)s, SP_SYMBOL)

#define SUFFICIENT_FEE SCORUM_MIN_ACCOUNT_CREATION_FEE* SCORUM_CREATE_ACCOUNT_WITH_SCORUM_MODIFIER

// test case defines
#if BOOST_VERSION >= 106000 // boost ver. >= 1.60.0

#define SCORUM_AUTO_TU_REGISTRAR(test_name)                                                                            \
    BOOST_AUTO_TU_REGISTRAR(test_name)                                                                                 \
    (boost::unit_test::make_test_case(&BOOST_AUTO_TC_INVOKER(test_name), #test_name, __FILE__, __LINE__),              \
     boost::unit_test::decorator::collector::instance());

#else // boost ver. <1.60.0

#define SCORUM_AUTO_TU_REGISTRAR(test_name)                                                                            \
    BOOST_AUTO_TU_REGISTRAR(test_name)                                                                                 \
    (boost::unit_test::make_test_case(&BOOST_AUTO_TC_INVOKER(test_name), #test_name),                                  \
     boost::unit_test::ut_detail::auto_tc_exp_fail<BOOST_AUTO_TC_UNIQUE_ID(test_name)>::instance()->value());

#endif

#define SCORUM_MESSAGE(M)                                                                                              \
    BOOST_MESSAGE(M);                                                                                                  \
    ilog(M)

#define SCORUM_TEST_CASE(test_name)                                                                                    \
    struct test_name : public BOOST_AUTO_TEST_CASE_FIXTURE                                                             \
    {                                                                                                                  \
        void test_method()                                                                                             \
        {                                                                                                              \
            try                                                                                                        \
            {                                                                                                          \
                test_method_override();                                                                                \
            }                                                                                                          \
            FC_LOG_AND_RETHROW()                                                                                       \
        };                                                                                                             \
        void test_method_override();                                                                                   \
    };                                                                                                                 \
                                                                                                                       \
    static void BOOST_AUTO_TC_INVOKER(test_name)()                                                                     \
    {                                                                                                                  \
        BOOST_TEST_CHECKPOINT('"' << #test_name << "\" fixture entry.");                                               \
        test_name t;                                                                                                   \
        BOOST_TEST_CHECKPOINT('"' << #test_name << "\" entry.");                                                       \
        t.test_method();                                                                                               \
        BOOST_TEST_CHECKPOINT('"' << #test_name << "\" exit.");                                                        \
    }                                                                                                                  \
                                                                                                                       \
    struct BOOST_AUTO_TC_UNIQUE_ID(test_name)                                                                          \
    {                                                                                                                  \
    };                                                                                                                 \
                                                                                                                       \
    SCORUM_AUTO_TU_REGISTRAR(test_name)                                                                                \
                                                                                                                       \
    void test_name::test_method_override()
