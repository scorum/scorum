add_executable( get_dev_key
                get_dev_key.cpp )

target_link_libraries( get_dev_key
                       PRIVATE
                       scorum_app
                       scorum_chain
                       scorum_protocol
                       graphene_utilities
                       fc
                       ${CMAKE_DL_LIBS}
                       ${PLATFORM_SPECIFIC_LIBS} )

install( TARGETS
   get_dev_key

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)


add_executable( test_shared_mem
                test_shared_mem.cpp )

target_link_libraries( test_shared_mem
                       PRIVATE
                       scorum_app
                       scorum_chain
                       scorum_protocol
                       graphene_utilities
                       fc
                       ${CMAKE_DL_LIBS}
                       ${PLATFORM_SPECIFIC_LIBS} )

add_executable( sign_digest
                sign_digest.cpp )

target_link_libraries( sign_digest
                       PRIVATE
                       scorum_app
                       scorum_chain
                       scorum_protocol
                       graphene_utilities
                       fc
                       ${CMAKE_DL_LIBS}
                       ${PLATFORM_SPECIFIC_LIBS} )

install( TARGETS
   sign_digest

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)

#add_executable( sign_transaction sign_transaction.cpp )

#target_link_libraries( sign_transaction
#                       PRIVATE scorum_app scorum_chain scorum_protocol graphene_utilities fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} )

#install( TARGETS
#   sign_transaction

#   RUNTIME DESTINATION bin
#   LIBRARY DESTINATION lib
#   ARCHIVE DESTINATION lib
#)

#add_executable( schema_test schema_test.cpp )
#target_link_libraries( schema_test
#                       PRIVATE scorum_chain fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} )
#
#install( TARGETS
#   schema_test
#
#   RUNTIME DESTINATION bin
#   LIBRARY DESTINATION lib
#   ARCHIVE DESTINATION lib
#)

add_executable( test_block_log
                test_block_log.cpp )
target_link_libraries( test_block_log
                       PRIVATE
                       scorum_chain
                       scorum_protocol
                       fc
                       ${CMAKE_DL_LIB}
                       ${PLATFORM_SPECIFIC_LIBS} )

install( TARGETS
   test_block_log

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)

add_executable( test_fixed_string
                test_fixed_string.cpp )
target_link_libraries( test_fixed_string
                       PRIVATE
                       scorum_chain
                       scorum_protocol
                       fc
                       ${CMAKE_DL_LIB}
                       ${PLATFORM_SPECIFIC_LIBS} )

install( TARGETS
   test_fixed_string

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)

add_executable( test_sqrt
                test_sqrt.cpp )
target_link_libraries( test_sqrt
                       PRIVATE
                       fc
                       ${CMAKE_DL_LIBS}
                       ${PLATFORM_SPECIFIC_LIBS} )
install( TARGETS
   test_sqrt

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
