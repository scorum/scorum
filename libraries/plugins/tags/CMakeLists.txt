set (TAGS_HPP "include/scorum/tags/tags_plugin.hpp"
              "include/scorum/tags/tags_api.hpp"
              "include/scorum/tags/tags_api_impl.hpp"
              "include/scorum/tags/tags_api_objects.hpp"
              "include/scorum/tags/tags_objects.hpp"
              "include/scorum/tags/tags_service.hpp")

add_library(scorum_tags
            tags_plugin.cpp
            tags_api.cpp
            tags_service.cpp
            tags_api_objects.cpp
            ${TAGS_HPP})

target_link_libraries(scorum_tags
                      scorum_chain
                      scorum_protocol
                      scorum_app
                      scorum_utils
                      scorum_common_api)

target_include_directories(scorum_tags
                           PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

add_custom_target(scorum_tags_manifest SOURCES plugin.json)

install(TARGETS
   scorum_tags

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
