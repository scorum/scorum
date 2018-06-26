file(GLOB_RECURSE HEADERS "include/scorum/utils/*.hpp")

add_library(scorum_utils
        string_algorithm.cpp
        )

target_link_libraries(scorum_utils
        ICU::ICU
        )
target_include_directories(scorum_utils
        PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

install(TARGETS
        scorum_utils

        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        )
