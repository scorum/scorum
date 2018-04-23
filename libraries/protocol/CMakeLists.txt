file(GLOB HEADERS "include/scorum/protocol/*.hpp")

## SORT .cpp by most likely to change / break compile
add_library( scorum_protocol

             asset.cpp
             atomicswap_helper.cpp
             authority.cpp
             block.cpp
             comment.cpp
             config.cpp
             get_config.cpp
             operation_util_impl.cpp
             operations.cpp
             scorum_operations.cpp
             sign_state.cpp
             transaction.cpp
             types.cpp
             version.cpp
             proposal_operations.cpp

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
