file(GLOB HEADERS "include/scorum/plugins/raw_block/*.hpp")

add_library( scorum_raw_block
             ${HEADERS}
             raw_block_plugin.cpp
             raw_block_api.cpp
           )

target_link_libraries( scorum_raw_block scorum_app scorum_chain scorum_protocol fc )
target_include_directories( scorum_raw_block
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
