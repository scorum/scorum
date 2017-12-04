add_executable( cli_wallet main.cpp )
if( UNIX AND NOT APPLE )
  set(rt_library rt )
endif()

find_package( Gperftools QUIET )
if( GPERFTOOLS_FOUND )
    message( STATUS "Found gperftools; compiling cli_wallet with TCMalloc")
    list( APPEND PLATFORM_SPECIFIC_LIBS tcmalloc )
endif()

# I don't know why scorum_app is required twice in the following line, I just know the linker breaks if it isn't.
target_link_libraries( cli_wallet
                       PRIVATE scorum_app graphene_net scorum_chain scorum_egenesis_brief scorum_protocol graphene_utilities scorum_wallet scorum_private_message scorum_app scorum_follow fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} )

if(MSVC)
  set_source_files_properties( main.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
endif(MSVC)

install( TARGETS
   cli_wallet

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
