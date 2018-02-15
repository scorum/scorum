file(GLOB_RECURSE HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp")

set( SOURCES
  main.cpp
  plugin_ops.cpp
)

add_executable(plugin_tests
              ${SOURCES}
              ${HEADERS})
target_link_libraries(plugin_tests
                      common_test
                      scorum_app
                      scorum_egenesis_none
                      )
target_include_directories(plugin_tests PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
