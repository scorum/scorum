file(GLOB HEADERS "include/scorum/chain/*.hpp"
                  "include/scorum/chain/evaluators/*.hpp"
                  "include/scorum/chain/schema/*.hpp"
                  "include/scorum/chain/services/*.hpp"
                  "include/scorum/chain/util/*.hpp"
                  "include/scorum/chain/genesis/*.hpp")

if( MSVC )
   set( hardfork_hpp_file "${CMAKE_CURRENT_SOURCE_DIR}/include/scorum/chain/hardfork.hpp" )
   add_custom_target( build_hardfork_hpp
      COMMAND cat-parts "${CMAKE_CURRENT_SOURCE_DIR}/hardfork.d" ${hardfork_hpp_file} )
   add_dependencies( build_hardfork_hpp cat-parts )
else( MSVC )
   set( hardfork_hpp_file "${CMAKE_CURRENT_BINARY_DIR}/include/scorum/chain/hardfork.hpp" )
   add_custom_target( build_hardfork_hpp
      COMMAND "${CMAKE_SOURCE_DIR}/programs/build_helpers/cat_parts.py" "${CMAKE_CURRENT_SOURCE_DIR}/hardfork.d" ${hardfork_hpp_file} )
endif( MSVC )


set_source_files_properties( "${CMAKE_CURRENT_BINARY_DIR}/include/scorum/chain/hardfork.hpp" PROPERTIES GENERATED TRUE )

## SORT .cpp by most likely to change / break compile
add_library( scorum_chain

             # As database takes the longest to compile, start it first
             database.cpp
             fork_database.cpp
             database_witness_schedule.cpp

             services/dbs_base.cpp
             services/dbservice_dbs_factory.cpp
             services/account.cpp
             services/reward.cpp
             services/witness.cpp
             services/budget.cpp
             services/registration_pool.cpp
             services/registration_committee.cpp
             services/proposal.cpp
             services/atomicswap.cpp
             services/dynamic_global_property.cpp
             services/comment.cpp
             services/comment_vote.cpp
             services/escrow.cpp
             services/witness_vote.cpp
             services/decline_voting_rights_request.cpp
             services/withdraw_vesting_route.cpp
             services/vesting_delegation.cpp
             services/reward_fund.cpp

             data_service_factory.cpp

             evaluators/scorum_evaluators.cpp
             evaluators/proposal_create_evaluator.cpp

             schema/shared_authority.cpp
             block_log.cpp

             genesis/genesis.cpp
             genesis/initializators/initializators.cpp
             genesis/initializators/accounts_initializator.cpp
             genesis/initializators/founders_initializator.cpp
             genesis/initializators/witnesses_initializator.cpp
             genesis/initializators/global_property_initializator.cpp
             genesis/initializators/rewards_initializator.cpp
             genesis/initializators/registration_initializator.cpp
             genesis/initializators/witness_schedule_initializator.cpp
             genesis/initializators/registration_bonus_initializator.cpp
             genesis/initializators/steemit_bounty_account_initializator.cpp

             util/reward.cpp

             ${HEADERS}
             ${hardfork_hpp_file}
             "${CMAKE_CURRENT_BINARY_DIR}/include/scorum/chain/hardfork.hpp"
           )

add_dependencies( scorum_chain scorum_protocol build_hardfork_hpp )
target_link_libraries( scorum_chain
                       scorum_protocol
                       fc
                       chainbase
                       graphene_schema
                       ${PATCH_MERGE_LIB}
                       ${PLATFORM_SPECIFIC_LIBS})
target_include_directories( scorum_chain
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_BINARY_DIR}/include" )

if(MSVC)
  set_source_files_properties( database.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
endif(MSVC)

if(UNIX)
  set_source_files_properties( evaluators/scorum_evaluators.cpp PROPERTIES COMPILE_FLAGS "-Wno-sign-compare -Wno-maybe-uninitialized" )
endif(UNIX)

INSTALL( TARGETS
   scorum_chain

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
INSTALL( FILES ${HEADERS} DESTINATION "include/scorum/chain" )
