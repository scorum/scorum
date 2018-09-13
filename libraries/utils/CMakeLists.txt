file(GLOB_RECURSE HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp")

set(SOURCES
    string_algorithm.cpp)

add_library(scorum_utils
        ${SOURCES}
        ${HEADERS}
        )

target_link_libraries(scorum_utils
        ICU::ICU
        fc
        )
target_include_directories(scorum_utils
        PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

install(TARGETS
        scorum_utils

        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        )
