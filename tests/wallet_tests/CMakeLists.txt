file(GLOB_RECURSE HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp")

set( SOURCES
  main.cpp
  keys_tests.cpp
)

add_executable(wallet_tests
              ${SOURCES}
              ${HEADERS})
target_link_libraries(wallet_tests
                      common_test
                      scorum_app
                      scorum_egenesis_none
                      scorum_blockchain_history
                      scorum_wallet
                      graphene_utilities
                      )
target_include_directories(wallet_tests PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

