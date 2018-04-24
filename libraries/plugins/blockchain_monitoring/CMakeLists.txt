file(GLOB HEADERS "include/scorum/blockchain_monitoring/*.hpp")

add_library( scorum_blockchain_monitoring
             blockchain_monitoring_plugin.cpp
             blockchain_statistics_api.cpp
             node_monitoring_api.cpp
             schema/metrics.cpp
           )

target_link_libraries( scorum_blockchain_monitoring
                       scorum_chain
                       scorum_protocol
                       scorum_app )
target_include_directories( scorum_blockchain_monitoring
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../common_statistics/include")

add_custom_target( scorum_blockchain_monitoring_manifest SOURCES plugin.json)

install( TARGETS
   scorum_blockchain_monitoring

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
