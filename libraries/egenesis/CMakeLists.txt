add_library(scorum_egenesis_none
            egenesis_none.cpp
            include/scorum/egenesis/egenesis.hpp
)

target_link_libraries(scorum_egenesis_none
                      scorum_chain
                      fc
)

target_include_directories(scorum_egenesis_none PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

add_executable(embed_genesis
               embed_genesis.cpp
)

target_link_libraries(embed_genesis
    scorum_chain
    scorum_app
    scorum_egenesis_none
    fc
)

set(embed_genesis_args
    -t "${CMAKE_CURRENT_SOURCE_DIR}/egenesis_brief.cpp.tmpl---${CMAKE_CURRENT_BINARY_DIR}/egenesis_brief.cpp"
    -t "${CMAKE_CURRENT_SOURCE_DIR}/egenesis_full.cpp.tmpl---${CMAKE_CURRENT_BINARY_DIR}/egenesis_full.cpp"
)

message(STATUS "  SCORUM_EGENESIS_JSON:               ${SCORUM_EGENESIS_JSON}")

if(SCORUM_EGENESIS_JSON)
    list(APPEND embed_genesis_args --genesis-json "${SCORUM_EGENESIS_JSON}")
endif(SCORUM_EGENESIS_JSON)

MESSAGE(STATUS "embed_genesis_args: " ${embed_genesis_args})

OPTION( SCORUM_TEST_GENESIS_JSON "Apply test for generated genesis.json (ON OR OFF)" OFF )

set(scorum_input_genesis_root_dir ${CMAKE_CURRENT_SOURCE_DIR}/../../genesis)
set(scorum_input_genesis_testnet_dir ${scorum_input_genesis_root_dir}/testnet)
set(scorum_input_genesis_mainnet_dir ${scorum_input_genesis_root_dir}/mainnet)

if (SCORUM_GENESIS_TESTNET)
    set(scorum_input_genesis_dir ${scorum_input_genesis_testnet_dir})
else()
    set(scorum_input_genesis_dir ${scorum_input_genesis_mainnet_dir})
endif()

set(scorum_genesis_testnet_template_json ${scorum_input_genesis_testnet_dir}/genesis.tpl.json)
set(scorum_genesis_mainnet_template_json ${scorum_input_genesis_mainnet_dir}/genesis.tpl.json)
set(scorum_genesis_template_json ${scorum_input_genesis_dir}/genesis.tpl.json)
#users.json: use 'python -m json.tool new_raw_users.json users.json' to update this file
set(scorum_users_json ${scorum_input_genesis_dir}/users.json)
add_custom_target( genesis SOURCES
                   ${scorum_genesis_testnet_template_json}
                   ${scorum_genesis_mainnet_template_json})

set(create_genesis_args)
list(APPEND create_genesis_args --input-genesis-json "${scorum_genesis_template_json}")
list(APPEND create_genesis_args --import-json "${scorum_users_json}")
list(APPEND create_genesis_args --output-genesis-json "${SCORUM_EGENESIS_JSON}")
if (SCORUM_TEST_GENESIS_JSON)
    list(APPEND create_genesis_args --test-resut-genesis)
endif()

#set(SCORUM_REQUIRED_USERS_IN_GENESIS "" CACHE STRING "Users list that are checked in result genesis (empty by default)")

#if (NOT SCORUM_REQUIRED_USERS_IN_GENESIS STREQUAL "")
#    list(APPEND create_genesis_args --check-users "${SCORUM_REQUIRED_USERS_IN_GENESIS}")
#endif()

MESSAGE(STATUS "create_genesis_args: " ${create_genesis_args})

add_custom_command(
    OUTPUT
    "${SCORUM_EGENESIS_JSON}"
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND create_genesis ${create_genesis_args}
    DEPENDS
    ${scorum_genesis_template_json}
    ${scorum_users_json}
)

add_custom_command(
    OUTPUT
    "${CMAKE_CURRENT_BINARY_DIR}/egenesis_brief.cpp"
    "${CMAKE_CURRENT_BINARY_DIR}/egenesis_full.cpp"
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND embed_genesis ${embed_genesis_args}
    DEPENDS
    "${SCORUM_EGENESIS_JSON}"
    "${CMAKE_CURRENT_SOURCE_DIR}/egenesis_brief.cpp.tmpl"
    "${CMAKE_CURRENT_SOURCE_DIR}/egenesis_full.cpp.tmpl"
    embed_genesis
)

add_library(scorum_egenesis_brief "${CMAKE_CURRENT_BINARY_DIR}/egenesis_brief.cpp" include/scorum/egenesis/egenesis.hpp)
add_library(scorum_egenesis_full  "${CMAKE_CURRENT_BINARY_DIR}/egenesis_full.cpp" include/scorum/egenesis/egenesis.hpp)

target_link_libraries(scorum_egenesis_brief
                      scorum_chain
                      fc)
target_link_libraries(scorum_egenesis_full
                      scorum_chain
                      fc)

target_include_directories(scorum_egenesis_brief PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(scorum_egenesis_full PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

INSTALL(TARGETS
    embed_genesis
    scorum_egenesis_none
    scorum_egenesis_brief
    scorum_egenesis_full

    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
