file(GLOB HEADERS "include/scorum/follow/*.hpp")

add_library( scorum_follow
             follow_plugin.cpp
             follow_api.cpp
             follow_operations.cpp
             follow_evaluators.cpp
           )

target_link_libraries( scorum_follow scorum_chain scorum_protocol scorum_app )
target_include_directories( scorum_follow
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   scorum_follow

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
