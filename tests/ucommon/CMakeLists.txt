find_package(Gperftools QUIET)
if(GPERFTOOLS_FOUND)
    message(STATUS "Found gperftools; compiling tests with TCMalloc")
    list(APPEND PLATFORM_SPECIFIC_LIBS tcmalloc)
endif()

file(GLOB_RECURSE HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp")

set( SOURCES
  actor.cpp
  genesis.cpp
  logger.cpp
)

add_library(ucommon_test 
            ${SOURCES}
            ${HEADERS})
target_link_libraries(ucommon_test
                      HippoMocks
                      chainbase
                      scorum_chain
                      scorum_protocol
                      cli
                      fc
                      ${PLATFORM_SPECIFIC_LIBS}
                      )
target_include_directories(ucommon_test PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

