OPTION( BUILD_MONGODB_CLI "Use MangoDB client to parse (ON OR OFF)" OFF )
MESSAGE( STATUS "BUILD_MONGODB_CLI: ${BUILD_MONGODB_CLI}" )
if( BUILD_MONGODB_CLI  )
  SET( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DBUILD_MONGODB_CLI" )
  SET( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DBUILD_MONGODB_CLI" )

  find_package(libmongocxx REQUIRED)
  find_package(libbsoncxx REQUIRED)

  include_directories(${LIBMONGOCXX_INCLUDE_DIR})
  include_directories(${LIBBSONCXX_INCLUDE_DIR})
  include_directories("/usr/local/include/mongocxx/v_noabi")
  include_directories("/usr/local/include/bsoncxx/v_noabi")
  include_directories("/usr/local/include/libmongoc-1.0")
  include_directories("/usr/local/include/libbson-1.0")
  include_directories("/usr/local/lib")
endif()

file(GLOB_RECURSE HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp")

set( SOURCES
   main.cpp
   genesis_parser.cpp
   genesis_mapper.cpp
   file_parser.cpp
   mongo_parser.cpp
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
if( BUILD_MONGODB_CLI  )
    target_link_libraries(create_genesis ${LIBBSONCXX_LIBRARIES})
    target_link_libraries(create_genesis ${LIBMONGOCXX_LIBRARIES})

    if(UNIX)
        SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}  -Wl,-rpath,/usr/local/lib")
    endif(UNIX)
endif()

target_include_directories(create_genesis PUBLIC
                           "${CMAKE_CURRENT_SOURCE_DIR}")

