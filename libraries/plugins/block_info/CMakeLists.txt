file(GLOB HEADERS "include/scorum/plugins/block_info/*.hpp")

add_library( scorum_block_info
             ${HEADERS}
             block_info_plugin.cpp
             block_info_api.cpp
           )

target_link_libraries( scorum_block_info
                       scorum_app
                       scorum_chain
                       scorum_protocol
                       fc )
target_include_directories( scorum_block_info
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

add_custom_target( scorum_block_info_manifest SOURCES plugin.json)
