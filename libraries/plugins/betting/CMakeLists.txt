file(GLOB_RECURSE HEADERS "include/scorum/betting/*.hpp")

add_library( scorum_betting
        betting_api.cpp
        betting_plugin.cpp

        ${HEADERS}
        )

target_link_libraries( scorum_betting
        scorum_chain
        scorum_protocol
        scorum_app
        )
target_include_directories( scorum_betting
        PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

add_custom_target( scorum_betting_manifest SOURCES plugin.json)

install( TARGETS
        scorum_betting

        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        )
