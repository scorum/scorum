file(GLOB_RECURSE HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp")

set( SOURCES
    main.cpp
    block_tests.cpp
    chain_api_tests.cpp
    operation_tests.cpp
    escrow_transfer_operation_tests.cpp
    account_data_service_tests.cpp
    witness_data_service_tests.cpp
    operation_time_tests.cpp
    reward_service_tests.cpp
    rewards/vote_apply_tests.cpp
    rewards/comment_beneficiaries_apply_tests.cpp
    rewards/per_block_reward_distribution.cpp
    rewards/fifa_world_cup_2018_bounty_reward_fund_tests.cpp
    registration/registration_check_common.cpp
    registration/committee_service_tests.cpp
    registration/committee_operations_tests.cpp
    registration/create_by_committee_evaluator_tests.cpp
    registration/registration_service_tests.cpp
    registration/registration_helpers.cpp
    atomicswap/atomicswap_service_tests.cpp
    atomicswap/atomicswap_operations_tests.cpp
    budget/budget_service_tests.cpp
    budget/fund_budget_tests.cpp
    budget/budget_operations_tests.cpp
    budget/budget_check_common.cpp
    proposal/proposal_tests.cpp
    plugins/statistic/node_monitoring_tests.cpp
    plugins/statistic/statistic_tests.cpp
    plugins/statistic/account_statistic_tests.cpp
    plugins/tags/tags_tests.cpp
    plugins/blockchain_history_tests.cpp
    plugins/blockinfo_tests.cpp
    genesis_db_tests.cpp
    withdraw_scorumpower/old_tests.cpp
    withdraw_scorumpower/withdraw_scorumpower_check_common.cpp
    withdraw_scorumpower/route_from_account_to_account_tests.cpp
    withdraw_scorumpower/route_from_account_to_dev_pool_tests.cpp
    withdraw_scorumpower/route_from_dev_pool_to_account_tests.cpp
    withdraw_scorumpower/route_from_dev_pool_to_dev_pool_tests.cpp
    withdraw_scorumpower/lock_tests.cpp
)

add_executable(chain_tests
             ${SOURCES}
             ${HEADERS})
target_link_libraries(chain_tests
                      common_test
                      scorum_app
                      scorum_rewards_math
                      scorum_egenesis_none
                      scorum_account_statistics
                      scorum_blockchain_monitoring
                      scorum_blockchain_history
                      )
target_include_directories(chain_tests PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

SET( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSRC_DIR=${CMAKE_CURRENT_SOURCE_DIR}" )

if(MSVC)
    set_source_files_properties(serialization_tests.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
endif(MSVC)
