file(GLOB HEADERS "include/scorum/tags/*.hpp")

add_library( scorum_tags
             tags_plugin.cpp )

target_link_libraries( scorum_tags
                       scorum_chain
                       scorum_protocol
                       scorum_app )
target_include_directories( scorum_tags
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   scorum_tags

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
