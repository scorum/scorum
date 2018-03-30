file(GLOB HEADERS "include/scorum/blockchain_statistics/*.hpp")

add_library( scorum_blockchain_statistics
             blockchain_statistics_plugin.cpp
             blockchain_statistics_api.cpp
             metrics.cpp
           )

target_link_libraries( scorum_blockchain_statistics
                       scorum_chain
                       scorum_protocol
                       scorum_app )
target_include_directories( scorum_blockchain_statistics
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../common_statistics/include")

add_custom_target( scorum_blockchain_statistics_manifest SOURCES plugin.json)

install( TARGETS
   scorum_blockchain_statistics

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
