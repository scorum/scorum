file(GLOB HEADERS "include/scorum/blockchain_statistics/*.hpp")

add_library( scorum_blockchain_statistics
             blockchain_statistics_plugin.cpp
             blockchain_statistics_api.cpp
           )

target_link_libraries( scorum_blockchain_statistics scorum_chain scorum_protocol scorum_app )
target_include_directories( scorum_blockchain_statistics
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   scorum_blockchain_statistics

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)