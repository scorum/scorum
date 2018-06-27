file(GLOB HEADERS "include/scorum/chain/*.hpp"
                  "include/scorum/chain/evaluators/*.hpp"
                  "include/scorum/chain/schema/*.hpp"
                  "include/scorum/chain/services/*.hpp"
                  "include/scorum/chain/util/*.hpp"
                  "include/scorum/chain/genesis/*.hpp"
                  "include/scorum/chain/genesis/initializators/*.hpp"
                  "include/scorum/chain/database/*.hpp"
                  "include/scorum/chain/database/block_tasks/*.hpp"
                  "database/block_tasks/*.hpp")

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
             database/database.cpp
             database/fork_database.cpp
             database/database_witness_schedule.cpp

             services/account.cpp
             services/account_blogging_statistic.cpp
             services/atomicswap.cpp
             services/comment.cpp
             services/comment_vote.cpp
             services/dbs_base.cpp
             services/dbservice_dbs_factory.cpp
             services/decline_voting_rights_request.cpp
             services/dev_pool.cpp
             services/development_committee.cpp
             services/dynamic_global_property.cpp
             services/escrow.cpp
             services/proposal.cpp
             services/proposal_executor.cpp
             services/registration_committee.cpp
             services/registration_pool.cpp
             services/scorumpower_delegation.cpp
             services/withdraw_scorumpower.cpp
             services/withdraw_scorumpower_route.cpp
             services/withdraw_scorumpower_route_statistic.cpp
             services/witness.cpp
             services/witness_vote.cpp
             services/genesis_state.cpp
             services/account_registration_bonus.cpp
             services/blocks_story.cpp

             data_service_factory.cpp

             evaluators/scorum_evaluators.cpp
             evaluators/proposal_create_evaluator.cpp
             evaluators/proposal_vote_evaluator.cpp
             evaluators/set_withdraw_scorumpower_route_evaluators.cpp
             evaluators/withdraw_scorumpower_evaluator.cpp
             evaluators/committee_accessor.cpp
             evaluators/proposal_evaluators.cpp
             evaluators/registration_pool_evaluator.cpp
             evaluators/create_budget_evaluator.cpp
             evaluators/close_budget_evaluator.cpp

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
             genesis/initializators/dev_pool_initializator.cpp
             genesis/initializators/dev_committee_initialiazator.cpp

             database/block_tasks/block_tasks.cpp
             database/block_tasks/process_contracts_expiration.cpp
             database/block_tasks/process_funds.cpp
             database/block_tasks/process_comments_cashout.cpp
             database/block_tasks/process_vesting_withdrawals.cpp
             database/block_tasks/withdrawable_actors_impl.cpp
             database/block_tasks/comments_cashout_impl.cpp
             database/block_tasks/process_fifa_world_cup_2018_bounty_cashout.cpp
             database/block_tasks/process_fifa_world_cup_2018_bounty_initialize.cpp
             database/block_tasks/process_account_registration_bonus_expiration.cpp
             database/block_tasks/process_witness_reward_in_sp_migration.cpp

             database/process_user_activity.cpp

             ${HEADERS}
             ${hardfork_hpp_file}
             "${CMAKE_CURRENT_BINARY_DIR}/include/scorum/chain/hardfork.hpp"
           )

add_dependencies( scorum_chain scorum_protocol build_hardfork_hpp )
target_link_libraries( scorum_chain
                       scorum_protocol
                       scorum_rewards_math
                       scorum_account_identity
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
