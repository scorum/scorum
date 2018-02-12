file(GLOB_RECURSE HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp")

set( SOURCES
    main.cpp
    block_tests.cpp
    operation_tests.cpp
    serialization_tests.cpp
    escrow_transfer_operation_tests.cpp
    account_data_service_tests.cpp
    live_tests.cpp
    operation_time_tests.cpp
    reward_service_tests.cpp
    rewards/vote_apply.cpp
    rewards/comment_beneficiaries_apply.cpp
    registration_check/committee_service.cpp
    registration_check/registration_check_common.cpp
    registration_check/committee_operations.cpp
    registration_check/pool_service.cpp
    atomicswap_check/atomicswap_service.cpp
    atomicswap_check/atomicswap_operations.cpp
    budget_check/budget_service.cpp
    budget_check/fund_budget.cpp
    budget_check/budget_operations.cpp
    proposal/proposal_tests.cpp
    genesis_db_tests.cpp
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

if(MSVC)
    set_source_files_properties(serialization_tests.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
endif(MSVC)
