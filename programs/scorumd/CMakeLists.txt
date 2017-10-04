add_executable( scorumd main.cpp )
if( UNIX AND NOT APPLE )
  set(rt_library rt )
endif()

find_package( Gperftools QUIET )
if( GPERFTOOLS_FOUND )
    message( STATUS "Found gperftools; compiling scorumd with TCMalloc")
    list( APPEND PLATFORM_SPECIFIC_LIBS tcmalloc )
endif()

target_link_libraries( scorumd PRIVATE
   scorum_plugins
   scorum_mf_plugins
   scorum_app
   scorum_witness
   scorum_account_history
   scorum_chain
   scorum_protocol
   fc
   ${CMAKE_DL_LIBS}
   ${PLATFORM_SPECIFIC_LIBS}
)

install( TARGETS
   scorumd

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
