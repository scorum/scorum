file(GLOB HEADERS "include/scorum/account_by_key/*.hpp")

add_library( scorum_account_by_key
             account_by_key_plugin.cpp
             account_by_key_api.cpp
           )

target_link_libraries( scorum_account_by_key scorum_chain scorum_protocol scorum_app )
target_include_directories( scorum_account_by_key
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   scorum_account_by_key

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
