file(GLOB_RECURSE HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp")

set( SOURCES
    main.cpp
    block_tests.cpp
    operation_tests.cpp
    escrow_transfer_operation_tests.cpp
    account_data_service_tests.cpp
    operation_time_tests.cpp
    reward_service_tests.cpp
    rewards/vote_apply.cpp
    rewards/comment_beneficiaries_apply.cpp
    registration/committee_service.cpp
    registration/registration_check_common.cpp
    registration/committee_operations.cpp
    registration/pool_service.cpp
    atomicswap/atomicswap_service.cpp
    atomicswap/atomicswap_operations.cpp
    budget/budget_service.cpp
    budget/fund_budget.cpp
    budget/budget_operations.cpp
    budget/budget_check_common.cpp
    proposal/proposal_tests.cpp
    statistic/statistic_tests.cpp
    genesis_db_tests.cpp
    withdraw_vesting/old_tests.cpp
    withdraw_vesting/route_to_dev_pool.cpp
    withdraw_vesting/route_to_account.cpp
)

add_executable(chain_tests
             ${SOURCES}
             ${HEADERS})
target_link_libraries(chain_tests
                      common_test
                      scorum_app
                      scorum_egenesis_none
                      )
target_include_directories(chain_tests PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

SET( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSRC_DIR=${CMAKE_CURRENT_SOURCE_DIR}" )

if(MSVC)
    set_source_files_properties(serialization_tests.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
endif(MSVC)
