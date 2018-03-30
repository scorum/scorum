file(GLOB HEADERS "include/scorum/delayed_node/*.hpp")

add_library( scorum_delayed_node
             delayed_node_plugin.cpp
           )

target_link_libraries( scorum_delayed_node
                       scorum_chain
                       scorum_protocol
                       scorum_app )
target_include_directories( scorum_delayed_node
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

if(MSVC)
  set_source_files_properties( delayed_node_plugin.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
endif(MSVC)

add_custom_target( scorum_delayed_node_manifest SOURCES plugin.json)

install( TARGETS
   scorum_delayed_node

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
