# Defines Scorum library target.
project( Scorum )
cmake_minimum_required( VERSION 3.2 )

set( BLOCKCHAIN_NAME "Scorum" )

set( GUI_CLIENT_EXECUTABLE_NAME Scorum )
set( CUSTOM_URL_SCHEME "gcs" )
set( INSTALLER_APP_ID "68ad7005-8eee-49c9-95ce-9eed97e5b347" )

# http://stackoverflow.com/a/18369825
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.8)
        message(FATAL_ERROR "GCC version must DB at least 4.8!")
    endif()
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 3.3)
        message(FATAL_ERROR "Clang version must be at least 3.3!")
    endif()
endif()

list( APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules" )

set(CMAKE_EXPORT_COMPILE_COMMANDS "ON")
set(SCORUM_EGENESIS_JSON "${CMAKE_CURRENT_SOURCE_DIR}/genesis.json" )

#set (ENABLE_INSTALLER 1)
#set (USE_PCH 1)

if (USE_PCH)
  include (cotire)
endif(USE_PCH)

list( APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/libraries/fc/CMakeModules" )
list( APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/libraries/fc/GitVersionGen" )
include( GetGitRevisionDescription )
get_git_head_revision( GIT_REFSPEC GIT_SHA2 )

SET(BOOST_COMPONENTS)
LIST(APPEND BOOST_COMPONENTS thread
                             date_time
                             system
                             filesystem
                             program_options
                             signals
                             serialization
                             chrono
                             unit_test_framework
                             context
                             locale
                             coroutine)
SET( Boost_USE_STATIC_LIBS ON CACHE STRING "ON or OFF" )

OPTION( GENESIS_TESTNET "Build embedded genesis for TEST NET (ON OR OFF)" ON )
MESSAGE( STATUS "GENESIS_TESTNET: ${GENESIS_TESTNET}" )
if( GENESIS_TESTNET  )
  MESSAGE( STATUS "     " )
  MESSAGE( STATUS "             EMBED GENESIS FOR TEST NET             " )
  MESSAGE( STATUS "     " )
endif()

OPTION( LOW_MEMORY_NODE "Build source for low memory node (ON OR OFF)" OFF )
MESSAGE( STATUS "LOW_MEMORY_NODE: ${LOW_MEMORY_NODE}" )
if( LOW_MEMORY_NODE )
  MESSAGE( STATUS "     " )
  MESSAGE( STATUS "             CONFIGURING FOR LOW MEMORY NODE      " )
  MESSAGE( STATUS "     " )
  SET( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DIS_LOW_MEM" )
  SET( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DIS_LOW_MEM" )
endif()

OPTION( CLEAR_VOTES "Build source to clear old votes from memory" ON )
if( CLEAR_VOTES )
  MESSAGE( STATUS "   CONFIGURING TO CLEAR OLD VOTES FROM MEMORY" )
  SET( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DCLEAR_VOTES" )
  SET( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DCLEAR_VOTES" )
endif()

OPTION( SKIP_BY_TX_ID "Skip ordering operation history by transaction id (ON or OFF)" OFF )
MESSAGE( STATUS "SKIP_BY_TX_ID: ${SKIP_BY_TX_ID}" )
if( SKIP_BY_TX_ID )
   SET( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSKIP_BY_TX_ID" )
   SET( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DSKIP_BY_TX_ID" )
endif()

SET(BLOGGING_START_DATE "2018-06-11T09:00:00" CACHE STRING "Blogging start date")

if( "${BLOGGING_START_DATE}" MATCHES "^[0-9][0-9][0-9][0-9]-[0-1][0-9]-[0-3][0-9]T[0-2][0-9]:[0-5][0-9]:[0-5][0-9]$" )
    MESSAGE( STATUS "BLOGGING_START_DATE is come at ${BLOGGING_START_DATE}" )
    SET( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DBLOGGING_START_DATE=${BLOGGING_START_DATE}" )
    SET( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DBLOGGING_START_DATE=${BLOGGING_START_DATE}" )
endif()

SET(FIFA_WORLD_CUP_2018_BOUNTY_CASHOUT_DATE "2018-07-05T14:50:00" CACHE STRING "FIFA World Cup 2018 bounty cashout date")

if( "${FIFA_WORLD_CUP_2018_BOUNTY_CASHOUT_DATE}" MATCHES "^[0-9][0-9][0-9][0-9]-[0-1][0-9]-[0-3][0-9]T[0-2][0-9]:[0-5][0-9]:[0-5][0-9]$" )
    MESSAGE( STATUS "FIFA_WORLD_CUP_2018_BOUNTY_CASHOUT is come at ${FIFA_WORLD_CUP_2018_BOUNTY_CASHOUT_DATE}" )
    SET( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DFIFA_WORLD_CUP_2018_BOUNTY_CASHOUT_DATE=${FIFA_WORLD_CUP_2018_BOUNTY_CASHOUT_DATE}" )
    SET( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DFIFA_WORLD_CUP_2018_BOUNTY_CASHOUT_DATE=${FIFA_WORLD_CUP_2018_BOUNTY_CASHOUT_DATE}" )
endif()

SET( WITNESS_REWARD_MIGRATION_DATE_DESCRIPTION "Migration date to new witness SP reward (due to new algorithm)" )

if( GENESIS_TESTNET )
    SET(WITNESS_REWARD_MIGRATION_DATE "2018-05-24T10:00:00" CACHE STRING "${WITNESS_REWARD_MIGRATION_DATE_DESCRIPTION}")
else()
    SET(WITNESS_REWARD_MIGRATION_DATE "2018-07-12T00:00:00" CACHE STRING "${WITNESS_REWARD_MIGRATION_DATE_DESCRIPTION}")
endif()

if( "${WITNESS_REWARD_MIGRATION_DATE}" MATCHES "^[0-9][0-9][0-9][0-9]-[0-1][0-9]-[0-3][0-9]T[0-2][0-9]:[0-5][0-9]:[0-5][0-9]$" )
    MESSAGE( STATUS "WITNESS_REWARD_MIGRATION_DATE is come at ${WITNESS_REWARD_MIGRATION_DATE}" )
    SET( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DWITNESS_REWARD_MIGRATION_DATE=${WITNESS_REWARD_MIGRATION_DATE}" )
    SET( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DWITNESS_REWARD_MIGRATION_DATE=${WITNESS_REWARD_MIGRATION_DATE}" )
endif()

SET( LOCK_BUDGETS_API_OPTION_DESCRIPTION "Lock all budgets (create and close) API (ON or OFF)" )
if( GENESIS_TESTNET )
    OPTION(LOCK_BUDGETS_API "${LOCK_BUDGETS_API_OPTION_DESCRIPTION}" OFF)
else()
    OPTION(LOCK_BUDGETS_API "${LOCK_BUDGETS_API_OPTION_DESCRIPTION}" ON)
endif()

MESSAGE( STATUS "LOCK_BUDGETS_API: ${LOCK_BUDGETS_API}" )
if( LOCK_BUDGETS_API )
    SET( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DLOCK_BUDGETS_API" )
    SET( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DLOCK_BUDGETS_API" )
endif()

SET( FORCE_UNLOCK_BLOGGING_API_OPTION_DESCRIPTION "Unlock blogging API operations despite start date (ON or OFF)" )
if( GENESIS_TESTNET )
    OPTION(FORCE_UNLOCK_BLOGGING_API "${FORCE_UNLOCK_BLOGGING_API_OPTION_DESCRIPTION}" ON)
else()
    OPTION(FORCE_UNLOCK_BLOGGING_API "${FORCE_UNLOCK_BLOGGING_API_OPTION_DESCRIPTION}" OFF)
endif()

MESSAGE( STATUS "FORCE_UNLOCK_BLOGGING_API: ${FORCE_UNLOCK_BLOGGING_API}" )
if( FORCE_UNLOCK_BLOGGING_API )
    SET( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DFORCE_UNLOCK_BLOGGING_API" )
    SET( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DFORCE_UNLOCK_BLOGGING_API" )
endif()

IF( WIN32 )
  SET(BOOST_ROOT $ENV{BOOST_ROOT})
  set(Boost_USE_MULTITHREADED ON)
  set(BOOST_ALL_DYN_LINK OFF) # force dynamic linking for all libraries
ENDIF(WIN32)

FIND_PACKAGE(Boost 1.58 REQUIRED COMPONENTS ${BOOST_COMPONENTS})

if( WIN32 )

   message( STATUS "Configuring Scorum on WIN32")
   set( DB_VERSION 60 )
   set( BDB_STATIC_LIBS 1 )

   set( ZLIB_LIBRARIES "" )
   SET( DEFAULT_EXECUTABLE_INSTALL_DIR bin/ )

   set(CRYPTO_LIB)

   if( MSVC )
      add_compile_options(/wd4503 /wd4267 /wd4244)
      #looks like this flag can have different default on some machines.
      SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /SAFESEH:NO")
      SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SAFESEH:NO")

      # Probably cmake has a bug and vcxproj generated for executable in Debug conf. has disabled debug info
      set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /DEBUG")

      # On windows tcl should be installed to the directory pointed by setenv.bat script
      SET(TCL_INCLUDE_PATH $ENV{TCL_ROOT}/include)
      MESSAGE(STATUS "tcl INCLUDE PATH: ${TCL_INCLUDE_PATH}")

      FIND_PACKAGE(TCL)
      MESSAGE(STATUS "tcl_library: ${TCL_LIBRARY}")

      SET(TCL_LIBS "optimized;${TCL_LIBRARY};debug;")
      get_filename_component(TCL_LIB_PATH "${TCL_LIBRARY}" PATH)
      get_filename_component(TCL_LIB_NAME "${TCL_LIBRARY}" NAME_WE)
      get_filename_component(TCL_LIB_EXT "${TCL_LIBRARY}" EXT)

      SET(TCL_LIBS "${TCL_LIBS}${TCL_LIB_PATH}/${TCL_LIB_NAME}g${TCL_LIB_EXT}")
      SET(TCL_LIBRARY ${TCL_LIBS})

   elseif( MINGW )
      SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fpermissive -msse4.2 -Wa,-mbig-obj")
      SET(CMAKE_CXX_FLAGS_RELEASE "-O3")
      # Optimization flag apparently needed to get rid of "File too big" assembler errors when compiling in Debug mode
      # See: https://stackoverflow.com/questions/14125007/gcc-string-table-overflow-error-during-compilation/14601779#29479701
      SET(CMAKE_CXX_FLAGS_DEBUG "-O2")
      if ( FULL_STATIC_BUILD )
        set( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libstdc++ -static-libgcc")
      endif ( FULL_STATIC_BUILD )
   endif( MSVC )

else( WIN32 ) # Apple AND Linux

    find_library(READLINE_LIBRARIES NAMES readline)
    find_path(READLINE_INCLUDE_DIR readline/readline.h)
    #if(NOT READLINE_INCLUDE_DIR OR NOT READLINE_LIBRARIES)
    #  MESSAGE(FATAL_ERROR "Could not find lib readline.")
    #endif()

    if( APPLE )
        # Apple Specific Options Here
        message( STATUS "Configuring Scorum on OS X" )
        set( CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=c++11 -stdlib=libc++ -Wall -Wno-conversion -Wno-deprecated-declarations -Wno-invalid-partial-specialization" )
    else( APPLE )
        # Linux Specific Options Here
        message( STATUS "Configuring Scorum on Linux" )
        set( CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=c++11 -Wall -Werror" )
        set( rt_library rt )
        set( pthread_library pthread)

        if ( NOT DEFINED crypto_library )
          # I'm not sure why this is here, I guess someone has openssl and can't detect it with find_package()?
          # if you have a normal install, you can define crypto_library to the empty string to avoid a build error
          set( crypto_library crypto)
        endif ()

        if ( FULL_STATIC_BUILD )
          set( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libstdc++ -static-libgcc")
        endif ( FULL_STATIC_BUILD )

    endif( APPLE )

    if( "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" )
        set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-builtin-memcmp" )
    endif()

    if( "${CMAKE_GENERATOR}" STREQUAL "Ninja" )
        if( "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" )
            set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcolor-diagnostics" )
        endif()
    endif()

    set( CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")

    # based on http://www.delorie.com/gnu/docs/gdb/gdb_70.html
    # uncomment this line to tell GDB about macros (slows compile times)
    # set( CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -gdwarf-2 -g3" )

endif( WIN32 )

set(ENABLE_COVERAGE_TESTING FALSE CACHE BOOL "Build Scorum for code coverage analysis")

if(ENABLE_COVERAGE_TESTING)
    SET(CMAKE_CXX_FLAGS "--coverage ${CMAKE_CXX_FLAGS}")
endif()

# external_plugins needs to be compiled first because libraries/app depends on SCORUM_EXTERNAL_PLUGINS being fully populated
add_subdirectory( external_plugins )
add_subdirectory( libraries )
add_subdirectory( programs )
add_subdirectory( tests )

if (ENABLE_INSTALLER)

set(VERSION_MAJOR 0)
set(VERSION_MINOR 1)
set(VERSION_PATCH 0)


include(InstallRequiredSystemLibraries)

set(CPACK_OUTPUT_FILE_PREFIX ${CMAKE_BINARY_DIR}/packages)
set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/install)

SET(CPACK_PACKAGE_DIRECTORY "${CMAKE_INSTALL_PREFIX}")
set(CPACK_PACKAGE_NAME "scorum")
set(CPACK_PACKAGE_VENDOR "Scorum, Inc.")
set(CPACK_PACKAGE_VERSION_MAJOR "${VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${VERSION_PATCH}")
set(CPACK_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")
set(CPACK_PACKAGE_DESCRIPTION "A client for the Scorum network")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A client for the Scorum network")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.md")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "Scorum ${CPACK_PACKAGE_VERSION}")

if(WIN32)
 SET(CPACK_GENERATOR "ZIP;NSIS")
 set(CPACK_PACKAGE_NAME "Scorum") # override above
 set(CPACK_NSIS_EXECUTABLES_DIRECTORY .)
 set(CPACK_NSIS_PACKAGE_NAME "Scorum v${CPACK_PACKAGE_VERSION}")
 set(CPACK_NSIS_DISPLAY_NAME "${CPACK_NSIS_PACKAGE_NAME}")
 set(CPACK_NSIS_DEFINES "  !define MUI_STARTMENUPAGE_DEFAULTFOLDER \\\"Scorum\\\"")
 # it seems like windows zip files usually don't have a single directory inside them, unix tgz frequently do
 SET(CPACK_INCLUDE_TOPLEVEL_DIRECTORY 0)

endif(WIN32)

if(APPLE)
  set(CPACK_GENERATOR "DragNDrop")
endif()

if(LINUX)
  # Linux gets a .tgz
  SET(CPACK_GENERATOR "TGZ")
  SET(CPACK_INCLUDE_TOPLEVEL_DIRECTORY 1)
endif(LINUX)

 include(CPack)
endif(ENABLE_INSTALLER)

if( GENESIS_TESTNET )
  MESSAGE( STATUS "\n\n             EMBEDDED GENESIS FOR TEST NETWORK             \n\n" )
else()
  MESSAGE( STATUS "\n\n             EMBEDDED GENESIS FOR MAIN NETWORK             \n\n" )
endif()

if( LOW_MEMORY_NODE )
  MESSAGE( STATUS "\n\n             CONFIGURED FOR LOW MEMORY NODE          \n\n" )
else()
  MESSAGE( STATUS "\n\n             CONFIGURED FOR FULL NODE                \n\n" )
endif()
