find_package(Gperftools QUIET)
if(GPERFTOOLS_FOUND)
    message(STATUS "Found gperftools; compiling tests with TCMalloc")
    list(APPEND PLATFORM_SPECIFIC_LIBS tcmalloc)
endif()

set( SOURCES
  actor.cpp
  database_fixture.cpp
)

set( HEADERS
  actor.hpp
  database_fixture.hpp
  defines.hpp
  genesis.hpp
)

add_library(common_test 
            ${SOURCES}
            ${HEADERS})
target_link_libraries(common_test
                      HippoMocks
                      chainbase
                      scorum_chain
                      scorum_protocol
                      scorum_account_history
                      scorum_witness
                      scorum_debug_node
                      cli
                      fc
                      ${PLATFORM_SPECIFIC_LIBS}
                      )
target_include_directories(common_test PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

