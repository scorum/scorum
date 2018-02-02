file(GLOB HEADERS "include/scorum/protocol/*.hpp")
file(GLOB SRC_FILES *.cpp)

add_library( scorum_protocol
             ${SRC_FILES}
             ${HEADERS}
           )

target_link_libraries( scorum_protocol
                       fc
                       ${PLATFORM_SPECIFIC_LIBS})
target_include_directories( scorum_protocol
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_BINARY_DIR}/include" )

INSTALL( TARGETS
   scorum_protocol

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)

INSTALL( FILES ${HEADERS} DESTINATION "include/scorum/protocol" )
