file(GLOB HEADERS "include/scorum/plugins/debug_node/*.hpp")

add_library( scorum_debug_node
             ${HEADERS}
             debug_node_plugin.cpp
             debug_node_api.cpp
           )

target_link_libraries( scorum_debug_node
                       scorum_app
                       scorum_chain
                       scorum_protocol
                       fc )
target_include_directories( scorum_debug_node
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS scorum_debug_node
         RUNTIME DESTINATION bin
         LIBRARY DESTINATION lib
         ARCHIVE DESTINATION lib
       )
INSTALL( FILES ${HEADERS} DESTINATION "include/scorum/plugins/debug_node" )
