file(GLOB_RECURSE HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp")

set( SOURCES
   main.cpp
   genesis_parser.cpp
   genesis_mapper.cpp
   file_parser.cpp
   genesis_tester.cpp
   )

add_executable(create_genesis
              ${SOURCES}
              ${HEADERS})
target_link_libraries(create_genesis
                      scorum_app
                      scorum_egenesis_none
                      scorum_chain
                      scorum_protocol
                      cli
                      fc
                      )

target_include_directories(create_genesis PUBLIC
                           "${CMAKE_CURRENT_SOURCE_DIR}")
