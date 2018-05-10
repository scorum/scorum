file(GLOB_RECURSE HEADERS "include/scorum/account_identity/*.hpp")

add_library( scorum_account_identity
            impacted.cpp
            owned.cpp
           )

target_link_libraries( scorum_account_identity
                       scorum_protocol
                       fc
                       )
target_include_directories( scorum_account_identity
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

install( TARGETS
   scorum_account_identity

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
