cmake_minimum_required (VERSION 2.8.4)
project (DynaHack C)

if (WIN32)
    set(INSTALL_BASE "$ENV{USERPROFILE}")
else ()
    set(INSTALL_BASE "$ENV{HOME}")
endif ()

# default installation paths in $HOME/dynahack
if (NOT BINDIR)
    set (BINDIR "${INSTALL_BASE}/dynahack/dynahack-data"
         CACHE PATH "binary location" )
endif()

if (NOT LIBDIR)
    set (LIBDIR "${INSTALL_BASE}/dynahack/dynahack-data"
         CACHE PATH "library location" )
endif()

if (NOT DATADIR)
    set (DATADIR "${INSTALL_BASE}/dynahack/dynahack-data"
         CACHE PATH "data location" )
endif()

if (NOT SHELLDIR)
    set (SHELLDIR "${INSTALL_BASE}/dynahack"
         CACHE PATH "launch script location" )
endif()

set (ALL_STATIC FALSE CACHE BOOL "Fully static build")
if (NOT ALL_STATIC)
    set (LIB_TYPE SHARED)
else ()
    if (UNIX AND NOT CYGWIN)
	# Don't require object files to be recompiled
	# if switching to a dynamic build.
	add_definitions(-fPIC)
    endif ()
    add_definitions(-DSTATIC_BUILD)
    set (LIB_TYPE STATIC)
endif ()

# DynaHack server currently uses several Linux-specific apis.
# Generalizing the code to the point where it will work on other systems
# is not impossible, but the work has not been done.
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    set (ENABLE_SERVER FALSE CACHE BOOL "Build the DynaHack network server")
else ()
    set (ENABLE_SERVER FALSE)
endif ()

set (ENABLE_NETCLIENT TRUE CACHE BOOL "Enable network client mode")

if (CMAKE_COMPILER_IS_GNUCC)
    set (CMAKE_C_FLAGS_DEBUG "-Wall -g3 -Wold-style-definition -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -Wpointer-arith -Wformat-security")
    if (UNIX AND NOT CYGWIN)
	add_definitions (-fvisibility=hidden)
    endif ()
endif ()

# dynahack core
add_subdirectory (libnitrohack)

# dynahack network client
if (ENABLE_NETCLIENT)
    add_subdirectory (libnitrohack_client)
endif ()

# dynahack text ui
add_subdirectory (nitrohack)

if (ENABLE_SERVER)
    add_subdirectory (nitrohack_server)
endif ()
