file(GLOB HEADERS "include/scorum/market_history/*.hpp")

add_library( scorum_market_history
             market_history_plugin.cpp
             market_history_api.cpp
           )

target_link_libraries( scorum_market_history scorum_chain scorum_protocol scorum_app )
target_include_directories( scorum_market_history
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   scorum_market_history

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)