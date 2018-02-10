OPTION( BUILD_PRINT_TEST_CLI_OUTPUT "Print output in CLI tests (ON OR OFF)" OFF )
MESSAGE( STATUS "BUILD_PRINT_TEST_CLI_OUTPUT: ${BUILD_PRINT_TEST_CLI_OUTPUT}" )
if( BUILD_PRINT_TEST_CLI_OUTPUT  )
  SET( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DPRINT_TEST_CLI_OUTPUT" )
  SET( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DPRINT_TEST_CLI_OUTPUT" )
endif()

file(GLOB_RECURSE HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp")

set( SOURCES
  main.cpp
  basic_tests.cpp
  genesis_tests.cpp
  print_tests.cpp
  atomicswap_check/atomicswap_helper_check.cpp
  proposal/proposal_create_evaluator_tests.cpp
  proposal_vote_operation_tests.cpp
  serialization_tests.cpp
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
target_include_directories(utests PUBLIC
                           "${CMAKE_CURRENT_SOURCE_DIR}"
                           "../common")
