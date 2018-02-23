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
  genesis/initializator.cpp
  genesis/accounts.cpp
  genesis/founders.cpp
  serialization_tests.cpp
  signed_transaction_serialization_tests.cpp
  tasks_base_tests.cpp
  block_tasks_tests.cpp
)

add_executable(utests
              ${SOURCES}
              ${HEADERS})
target_link_libraries(utests
                      ucommon_test
                      scorum_app
                      scorum_egenesis_none
                      )
target_include_directories(utests PUBLIC
                           "${CMAKE_CURRENT_SOURCE_DIR}")

if(UNIX)
    set_source_files_properties( genesis/initializator.cpp PROPERTIES COMPILE_FLAGS "-Wno-uninitialized" )
endif(UNIX)
