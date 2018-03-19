CMAKE_MINIMUM_REQUIRED( VERSION 2.8.12 )

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules")

# begin readline stuff
find_package(Curses)
find_package(Readline)

MESSAGE(STATUS "Configuring CLI lib")

if (READLINE_FOUND)
    SET( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DHAVE_READLINE" )
    SET( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DHAVE_READLINE" )
    set(readline_libraries ${Readline_LIBRARY})
    if (CURSES_FOUND)
        list(APPEND readline_libraries ${CURSES_LIBRARY})
    endif()
    set(readline_includes ${Readline_INCLUDE_DIR})
endif()

file(GLOB HEADERS "include/scorum/cli/*.hpp")

## SORT .cpp by most likely to change / break compile
add_library( cli
             formatter.cpp
             app.cpp
             completionregistry.cpp
             ${HEADERS}
           )

target_link_libraries( cli
                       fc
                       ${PLATFORM_SPECIFIC_LIBS}
                       ${readline_libraries})
target_include_directories( cli
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_BINARY_DIR}/include" )

