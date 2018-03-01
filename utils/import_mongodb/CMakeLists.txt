
find_package(libmongocxx REQUIRED)
find_package(libbsoncxx REQUIRED)

include_directories(${LIBMONGOCXX_INCLUDE_DIR})
include_directories(${LIBBSONCXX_INCLUDE_DIR})
include_directories("/usr/local/include/mongocxx/v_noabi")
include_directories("/usr/local/include/bsoncxx/v_noabi")
include_directories("/usr/local/include/libmongoc-1.0")
include_directories("/usr/local/include/libbson-1.0")
include_directories("/usr/local/lib")

file(GLOB_RECURSE HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp")

set( SOURCES
   main.cpp )

add_executable(import_mongodb
              ${SOURCES}
              ${HEADERS})
target_link_libraries(import_mongodb
                      scorum_chain
                      scorum_protocol
                      cli
                      fc
                      )

target_link_libraries(import_mongodb ${LIBBSONCXX_LIBRARIES})
target_link_libraries(import_mongodb ${LIBMONGOCXX_LIBRARIES})

if(UNIX)
    SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}  -Wl,-rpath,/usr/local/lib")
endif(UNIX)

target_include_directories(import_mongodb PUBLIC
                           "${CMAKE_CURRENT_SOURCE_DIR}")

