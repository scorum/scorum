set( SOURCES
  main.cpp
)

add_executable(utests
              ${SOURCES}
              ${HEADERS})
target_link_libraries(utests
                      HippoMocks
                      chainbase
                      scorum_chain
                      scorum_protocol
                      scorum_app
                      scorum_egenesis_none
                      cli
                      fc
                      ${PLATFORM_SPECIFIC_LIBS}
                      )
target_include_directories(utests PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
