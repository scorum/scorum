file(GLOB HEADERS "include/scorum/protocol/*.hpp")

## SORT .cpp by most likely to change / break compile
add_library( scorum_protocol

             types.cpp
             authority.cpp
             operations.cpp
             sign_state.cpp
             operation_util_impl.cpp
             scorum_operations.cpp
             transaction.cpp
             block.cpp
             asset.cpp
             version.cpp
             get_config.cpp
             atomicswap_helper.cpp

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
