file(GLOB HEADERS "include/scorum/chain/*.hpp" "include/scorum/chain/util/*.hpp")

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
             witness_schedule.cpp

             scorum_evaluator.cpp

             scorum_objects.cpp
             shared_authority.cpp
             block_log.cpp

             util/reward.cpp

             ${HEADERS}
             ${hardfork_hpp_file}
             "${CMAKE_CURRENT_BINARY_DIR}/include/scorum/chain/hardfork.hpp"
           )

add_dependencies( scorum_chain scorum_protocol build_hardfork_hpp )
target_link_libraries( scorum_chain scorum_protocol fc chainbase graphene_schema ${PATCH_MERGE_LIB} )
target_include_directories( scorum_chain
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_BINARY_DIR}/include" )

if(MSVC)
  set_source_files_properties( database.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
endif(MSVC)

INSTALL( TARGETS
   scorum_chain

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
INSTALL( FILES ${HEADERS} DESTINATION "include/scorum/chain" )
