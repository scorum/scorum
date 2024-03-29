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
    accounts/delegate_sp_from_reg_pool_tests.cpp
    proposal/proposal_operations_tests.cpp
    proposal/proposal_evaluator_register_tests.cpp
    proposal/proposal_create_evaluator_tests.cpp
    proposal/proposal_vote_evaluator_tests.cpp
    proposal/development_committee_transfer_evaluator_tests.cpp
    proposal/development_committee_top_budgets_evaluator_tests.cpp
    proposal/development_committee_change_betting_moderator_tests.cpp
    rewards_math/calculate_payout_tests.cpp
    rewards_math/calculate_total_claims_tests.cpp
    rewards_math/calculate_curations_payout_tests.cpp
    rewards_math/calculate_weight_tests.cpp
    rewards_math/calculate_abs_reward_shares_tests.cpp
    rewards_math/calculate_voting_power_tests.cpp
    rewards/comment_reward_legacy_tests.cpp
    rewards/comment_reward_tests.cpp
    utils/string_algorithm_tests.cpp
    utils/take_n_range_tests.cpp
    utils/flatten_range_tests.cpp
    utils/join_range_tests.cpp
    utils/static_variant_comparison_tests.cpp
    fc/static_variant_visitor_tests.cpp
    utils/math_tests.cpp
    tasks_base_tests.cpp
    app_tests.cpp
    budgets/evaluators_tests.cpp
    budgets/auction_calculation_tests.cpp
    budgets/advertising_api_tests.cpp
    budgets/per_block_evaluation_tests.cpp
    budgets/serialization_tests.cpp
    fraction_tests.cpp
    config_api_tests.cpp
    vote_evaluator_tests.cpp
    betting/invariants_validation_tests.cpp
    betting/wincase_comparison_tests.cpp
    betting/threshold_comparison_tests.cpp
    betting/game_serialization_tests.cpp
    betting/betting_service_tests.cpp
    betting/betting_math_tests.cpp
    betting/betting_matcher_tests.cpp
    betting/bet_evaluators_tests.cpp
    betting/betting_api_tests.cpp
    betting/post_game_results_serialization_tests.cpp
    betting/wincase_type_tests.cpp
    betting/market_type_tests.cpp
    betting/create_matched_bet_tests.cpp
    betting/betting_chain_capital_tests.cpp
    betting/evaluators/create_game_tests.cpp
    betting/evaluators/cancel_game_tests.cpp
    betting/evaluators/update_game_markets_tests.cpp
    betting/evaluators/update_start_time_tests.cpp
    betting/evaluators/post_game_results_tests.cpp
    betting/betting_chain_capital_tests.cpp
    db_accessors/db_accessors_tests.cpp
    odds_tests.cpp
    create_account_by_committee_evaluator_tests.cpp
    nft/nft_evaluators_tests.cpp
    nft/nft_operations_tests.cpp
    nft/game_round_evaluators_tests.cpp
    nft/marshal_operation_tests.cpp
)

add_executable(utests
              ${SOURCES}
              ${HEADERS})

target_link_libraries(utests
                      fc
                      chainbase
                      common_test
                      ucommon_test
                      scorum_app
                      scorum_rewards_math
                      scorum_egenesis_none
                      scorum_utils)

target_include_directories(utests PUBLIC
                           "${CMAKE_CURRENT_SOURCE_DIR}")
