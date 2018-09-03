find_package(Gperftools QUIET)
if(GPERFTOOLS_FOUND)
    message(STATUS "Found gperftools; compiling tests with TCMalloc")
    list(APPEND PLATFORM_SPECIFIC_LIBS tcmalloc)
endif()

file(GLOB_RECURSE HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp")

set( SOURCES
  database_integration.cpp
  database_trx_integration.cpp
  database_default_integration.cpp
  database_blog_integration.cpp
  actoractions.cpp
)

add_library(common_test 
            ${SOURCES}
            ${HEADERS})
target_link_libraries(common_test
                      ucommon_test
                      HippoMocks
                      chainbase
                      scorum_chain
                      scorum_protocol
                      scorum_witness
                      scorum_debug_node
                      scorum_account_by_key
                      scorum_account_statistics
                      scorum_blockchain_history
                      scorum_blockchain_monitoring
                      scorum_tags
                      cli
                      fc
                      ${PLATFORM_SPECIFIC_LIBS}
                      )
target_include_directories(common_test PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

