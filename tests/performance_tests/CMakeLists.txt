file(GLOB_RECURSE HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp")

set( SOURCES
    main.cpp
    plugins/tags/get_discussions_by_tests.cpp
)

add_executable(performance_tests
             ${SOURCES}
             ${HEADERS})
target_link_libraries(performance_tests
                      common_test
                      scorum_app
                      scorum_rewards_math
                      scorum_egenesis_none
                      scorum_account_statistics
                      scorum_blockchain_monitoring
                      scorum_blockchain_history
                      )
target_include_directories(performance_tests PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

SET( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSRC_DIR=${CMAKE_CURRENT_SOURCE_DIR}" )

if(MSVC)
    set_source_files_properties(serialization_tests.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
endif(MSVC)
