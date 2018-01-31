file(GLOB HEADERS "include/scorum/ui/*.hpp")

## SORT .cpp by most likely to change / break compile
add_library( ui
             formatter.cpp
             ${HEADERS}
           )

target_link_libraries( ui
             fc
             ${CMAKE_DL_LIBS}
                          ${PLATFORM_SPECIFIC_LIBS} )
target_include_directories( ui
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_BINARY_DIR}/include" )

