set( SOURCES
  main.cpp
  plugin_ops.cpp
)

set( HEADERS
)

add_executable(plugin_test
              ${SOURCES}
              ${HEADERS})
target_link_libraries(plugin_test
                      common_test
                      scorum_app
                      scorum_egenesis_none
                      )
target_include_directories(plugin_test PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

