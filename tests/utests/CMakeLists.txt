OPTION( BUILD_PRINT_TEST_CLI_OUTPUT "Print output in CLI tests (ON OR OFF)" OFF )
MESSAGE( STATUS "BUILD_PRINT_TEST_CLI_OUTPUT: ${BUILD_PRINT_TEST_CLI_OUTPUT}" )
if( BUILD_PRINT_TEST_CLI_OUTPUT  )
  SET( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DPRINT_TEST_CLI_OUTPUT" )
  SET( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DPRINT_TEST_CLI_OUTPUT" )
endif()

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
    utils/string_algorithm_tests.cpp
    tasks_base_tests.cpp
    app_tests.cpp
    budgets/management_algorithms_tests.cpp
    budgets/evaluators_tests.cpp
    budgets/vcg_calculation_tests.cpp
    fraction_tests.cpp
    config_api_tests.cpp
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
