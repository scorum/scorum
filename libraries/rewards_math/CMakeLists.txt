file(GLOB_RECURSE HEADERS "include/scorum/rewards_math/*.hpp")

add_library( scorum_rewards_math
            curve.cpp
             formulas.cpp
           )

target_link_libraries( scorum_rewards_math
                       scorum_protocol
                       fc
                       )
target_include_directories( scorum_rewards_math
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

install( TARGETS
   scorum_rewards_math

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
