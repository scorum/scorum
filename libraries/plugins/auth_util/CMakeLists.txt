file(GLOB HEADERS "include/scorum/plugins/auth_util/*.hpp")

add_library( scorum_auth_util
             ${HEADERS}
             auth_util_plugin.cpp
             auth_util_api.cpp
           )

target_link_libraries( scorum_auth_util
                       scorum_app
                       scorum_chain
                       scorum_protocol
                       fc )
target_include_directories( scorum_auth_util
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

add_custom_target( scorum_auth_util_manifest SOURCES plugin.json)
