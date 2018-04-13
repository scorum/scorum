file(GLOB_RECURSE HEADERS "include/scorum/rewards/*.hpp")

add_library( scorum_rewards
             curve.cpp
             formulas.cpp
           )

target_link_libraries( scorum_rewards
                       scorum_protocol
                       fc
                       )
target_include_directories( scorum_rewards
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

install( TARGETS
   scorum_rewards

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
