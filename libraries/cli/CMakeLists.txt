file(GLOB HEADERS "include/scorum/cli/*.hpp")

## SORT .cpp by most likely to change / break compile
add_library( cli
             formatter.cpp
             ${HEADERS}
           )

target_link_libraries( cli
                       fc
                       ${PLATFORM_SPECIFIC_LIBS} )
target_include_directories( cli
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_BINARY_DIR}/include" )

