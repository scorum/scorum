find_package(Gperftools QUIET)
if(GPERFTOOLS_FOUND)
    message(STATUS "Found gperftools; compiling tests with TCMalloc")
    list(APPEND PLATFORM_SPECIFIC_LIBS tcmalloc)
endif()

file(GLOB_RECURSE HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp")

set( SOURCES
  actor.cpp
  database_fixture.cpp
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
                      scorum_blockchain_statistics
                      cli
                      fc
                      ${PLATFORM_SPECIFIC_LIBS}
                      )
target_include_directories(common_test PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

