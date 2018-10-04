OPTION(SCORUM_BUILD_PRINT_TEST_CLI_OUTPUT "Print output in CLI tests (ON OR OFF)" OFF )
OPTION(SCORUM_UTESTS_LOG_MESSAGES "Show logs for u-tests (ON or OFF)" ON)

if(SCORUM_BUILD_PRINT_TEST_CLI_OUTPUT)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DPRINT_TEST_CLI_OUTPUT")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DPRINT_TEST_CLI_OUTPUT")
endif()

if(SCORUM_UTESTS_LOG_MESSAGES)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DLOG_MESSAGES")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DLOG_MESSAGES")
endif()

message(STATUS "")
message(STATUS "  SCORUM_UTESTS_LOG_MESSAGES:         ${SCORUM_UTESTS_LOG_MESSAGES}")
message(STATUS "  SCORUM_BUILD_PRINT_TEST_CLI_OUTPUT: ${SCORUM_BUILD_PRINT_TEST_CLI_OUTPUT}")

file(GLOB_RECURSE HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp")

set(SOURCES
    main.cpp
    basic_tests.cpp
    genesis_tests.cpp
    print_tests.cpp
    atomicswap/atomicswap_helper_tests.cpp
    genesis/initializator_tests.cpp
    genesis/accounts_tests.cpp
    genesis/founders_tests.cpp
    logger/logger_config_tests.cpp
    signed_transaction_serialization_tests.cpp
    serialization_tests.cpp
    proposal/proposal_operations_tests.cpp
    proposal/proposal_evaluator_register_tests.cpp
    proposal/proposal_create_evaluator_tests.cpp
    proposal/proposal_vote_evaluator_tests.cpp
    proposal/development_committee_transfer_evaluator_tests.cpp
    proposal/development_committee_top_budgets_evaluator_tests.cpp
    rewards_math/calculate_payout_tests.cpp
    rewards_math/calculate_total_claims_tests.cpp
    rewards_math/calculate_curations_payout_tests.cpp
    rewards_math/calculate_weight_tests.cpp
    rewards_math/calculate_abs_reward_shares_tests.cpp
    rewards_math/calculate_voting_power_tests.cpp
    rewards/comment_reward_legacy_tests.cpp
    rewards/comment_reward_tests.cpp
    utils/string_algorithm_tests.cpp
    utils/math_tests.cpp
    tasks_base_tests.cpp
    app_tests.cpp
    budgets/evaluators_tests.cpp
    budgets/auction_calculation_tests.cpp
    budgets/advertising_api_tests.cpp
    fraction_tests.cpp
    config_api_tests.cpp
    vote_evaluator_tests.cpp
)

add_executable(utests
              ${SOURCES}
              ${HEADERS})

target_link_libraries(utests
                      ucommon_test
                      scorum_app
                      scorum_rewards_math
                      scorum_egenesis_none
                      scorum_utils)

target_include_directories(utests PUBLIC
                           "${CMAKE_CURRENT_SOURCE_DIR}")
