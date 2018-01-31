file(GLOB HEADERS "include/scorum/private_message/*.hpp")

add_library( scorum_private_message
             private_message_plugin.cpp
           )

target_link_libraries( scorum_private_message
                       scorum_chain
                       scorum_protocol
                       scorum_app )
target_include_directories( scorum_private_message
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   scorum_private_message

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
