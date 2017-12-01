find_package( Gperftools QUIET )
if( GPERFTOOLS_FOUND )
    message( STATUS "Found gperftools; compiling tests with TCMalloc")
    list( APPEND PLATFORM_SPECIFIC_LIBS tcmalloc )
endif()

file(GLOB HEADERS "common/*.hpp")
file(GLOB COMMON_SOURCES "common/*.cpp")
file(GLOB UNIT_TESTS "tests/*.cpp")
file(GLOB UNIT_BUDGET_TESTS "tests/budget_check/*.cpp")

add_executable( chain_test ${COMMON_SOURCES} ${UNIT_TESTS} ${UNIT_BUDGET_TESTS} ${HEADERS} )
target_link_libraries( chain_test chainbase scorum_chain scorum_protocol scorum_app scorum_account_history scorum_witness scorum_debug_node fc ${PLATFORM_SPECIFIC_LIBS} )

file(GLOB PLUGIN_TESTS "plugin_tests/*.cpp")
add_executable( plugin_test ${PLUGIN_TESTS} ${COMMON_SOURCES} )
target_link_libraries( plugin_test scorum_chain scorum_protocol scorum_app scorum_account_history scorum_witness scorum_debug_node fc ${PLATFORM_SPECIFIC_LIBS} )

if(MSVC)
  set_source_files_properties( tests/serialization_tests.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
endif(MSVC)

#add_subdirectory( generate_empty_blocks )
