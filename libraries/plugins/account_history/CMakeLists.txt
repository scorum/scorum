file(GLOB HEADERS "include/scorum/account_history/*.hpp")

add_library( scorum_account_history
             account_history_plugin.cpp
           )

target_link_libraries( scorum_account_history scorum_chain scorum_protocol scorum_app )
target_include_directories( scorum_account_history
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   scorum_account_history

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
