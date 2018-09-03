file(GLOB_RECURSE HEADERS "include/scorum/snapshot/*.hpp"
                          "impl/include/snapshot/*.hpp")

add_library( scorum_snapshot
             snapshot_plugin.cpp
             snapshot_helper.cpp
             impl/save/saving_task.cpp
             impl/save/save_file.cpp
             impl/load/loading_task.cpp
             impl/load/load_file.cpp
           )

target_link_libraries( scorum_snapshot
                       scorum_chain
                       scorum_protocol
                       scorum_app
                       scorum_witness
                       scorum_account_by_key
                       scorum_account_statistics
                       scorum_blockchain_history
                       scorum_blockchain_monitoring
                       scorum_tags
                       )
target_include_directories( scorum_snapshot
                            PRIVATE
                            "${CMAKE_CURRENT_SOURCE_DIR}/impl/include"
                            PUBLIC
                            "${CMAKE_CURRENT_SOURCE_DIR}/include")

add_custom_target( scorum_snapshot_manifest SOURCES plugin.json)

SET(SNAPSHOT_DEBUG_OBJECT "" CACHE STRING "Print debug info for certain database object")

if( "${SNAPSHOT_DEBUG_OBJECT}" MATCHES "^[a-z][_a-z]+[a-z]$" )
    MESSAGE( STATUS "SNAPSHOT_DEBUG_OBJECT is '${SNAPSHOT_DEBUG_OBJECT}'" )
    SET( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSNAPSHOT_DEBUG_OBJECT=${SNAPSHOT_DEBUG_OBJECT}" )
    SET( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DSNAPSHOT_DEBUG_OBJECT=${SNAPSHOT_DEBUG_OBJECT}" )
endif()

OPTION(SNAPSHOT_SAVE_MULTIPLE_SNAPSHOTS "Switch debug mode when node check files with block numbers names to make snapshot for each block number (ON or OFF)" OFF)

MESSAGE( STATUS "SNAPSHOT_SAVE_MULTIPLE_SNAPSHOTS: ${SNAPSHOT_SAVE_MULTIPLE_SNAPSHOTS}" )
if( SNAPSHOT_SAVE_MULTIPLE_SNAPSHOTS )
   SET( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSNAPSHOT_SAVE_MULTIPLE_SNAPSHOTS" )
   SET( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DSNAPSHOT_SAVE_MULTIPLE_SNAPSHOTS" )
endif()

install( TARGETS
   scorum_snapshot

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
