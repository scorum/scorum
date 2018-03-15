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
                      scorum_chain
                      scorum_protocol
                      scorum_egenesis_brief
                      cli
                      fc
                      )

                  set(create_genesis_args
                      -t "${CMAKE_CURRENT_SOURCE_DIR}/egenesis_brief.cpp.tmpl---${CMAKE_CURRENT_BINARY_DIR}/egenesis_brief.cpp"
                      -t "${CMAKE_CURRENT_SOURCE_DIR}/egenesis_full.cpp.tmpl---${CMAKE_CURRENT_BINARY_DIR}/egenesis_full.cpp"
                  )

target_include_directories(create_genesis PUBLIC
                           "${CMAKE_CURRENT_SOURCE_DIR}")

OPTION( TEST_GENESIS_JSON "Apply test for generated genesis.json (ON OR OFF)" OFF )

set(scorum_input_genesis_dir ${CMAKE_CURRENT_SOURCE_DIR}/../../genesis)
set(scorum_output_genesis_dir ${CMAKE_CURRENT_SOURCE_DIR}/../..)

set(scorum_genesis_template_json ${scorum_input_genesis_dir}/genesis.tpl.json)
#users.json: use 'python -m json.tool new_raw_users.json users.json' to update this file
set(scorum_users_json ${scorum_input_genesis_dir}/users.json)
set(scorum_genesis_json ${scorum_output_genesis_dir}/genesis.json)

add_custom_target( genesis SOURCES
                   ${scorum_genesis_template_json}
                   ${scorum_users_json})

set(create_genesis_args)
list(APPEND create_genesis_args --input-genesis-json "${scorum_genesis_template_json}")
list(APPEND create_genesis_args --import-json "${scorum_users_json}")
list(APPEND create_genesis_args --output-genesis-json "${scorum_genesis_json}")
if (TEST_GENESIS_JSON)
    list(APPEND create_genesis_args --test-resut-genesis)
endif()

MESSAGE(STATUS "create_genesis_args: " ${create_genesis_args})

add_custom_command(
   TARGET create_genesis POST_BUILD
   WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
   COMMAND create_genesis ${create_genesis_args}
)
