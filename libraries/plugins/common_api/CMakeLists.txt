file(GLOB_RECURSE HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp")

set(SOURCES
    config_api.cpp)

add_library( scorum_common_api
             ${SOURCES}
             ${HEADERS} )

target_link_libraries( scorum_common_api
                       fc)
target_include_directories( scorum_common_api
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
                            PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/include" )
