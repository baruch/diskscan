cmake_minimum_required(VERSION 3.10)
project(diskscan
        VERSION 0.21)

export(PACKAGE diskscan)

set(PACKAGE_VERSION ${PROJECT_VERSION})

# Default to a debug build
if(NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE Debug)
endif(NOT CMAKE_BUILD_TYPE)

# Pull in zlib
find_package(ZLIB REQUIRED)

# Ensure clock_gettime can build with or without -lrt as needed
include(CheckLibraryExists)
CHECK_LIBRARY_EXISTS(rt clock_gettime "time.h" HAVE_CLOCK_GETTIME)
if (NOT HAVE_CLOCK_GETTIME)
   set(CMAKE_EXTRA_INCLUDE_FILES time.h)
   CHECK_FUNCTION_EXISTS(clock_gettime HAVE_CLOCK_GETTIME)
   SET(CMAKE_EXTRA_INCLUDE_FILES)
else()
        list(APPEND LIBS rt)
endif()

# Find tinfo for termcap functions
INCLUDE (CheckIncludeFiles)
CHECK_INCLUDE_FILES(termcap.h HAVE_TERMCAP_H)
find_library(tinfo_LIBRARY NAMES tinfo curses)

# Architecture files
message("SYSTEM NAME: ${CMAKE_SYSTEM_NAME}")
if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
        set(ARCH_SRC "arch/arch-linux.c")
        set(ARCH_INCLUDE "arch/arch-linux.h")
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "kFreeBSD")
        set(ARCH_SRC "arch/arch-freebsd.c")
        set(ARCH_INCLUDE "arch/arch-posix.h")
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD")
        set(ARCH_SRC "arch/arch-freebsd.c")
        set(ARCH_INCLUDE "arch/arch-posix.h")
else()
        set(ARCH_SRC "arch/arch-generic.c")
        set(ARCH_INCLUDE "arch/arch-posix.h")
endif()

add_custom_command(OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/include/arch-internal.h
        COMMAND ln -fs ${CMAKE_CURRENT_SOURCE_DIR}/${ARCH_INCLUDE} ${CMAKE_CURRENT_SOURCE_DIR}/include/arch-internal.h
                  )

# Build diskscan
include_directories("include")
add_compile_options(-Wall -Wextra -Wshadow -Wmissing-prototypes -Winit-self)
add_definitions(-D_GNU_SOURCE -D_FORTIFY_SOURCE=2)
add_definitions(-DVERSION="${PROJECT_VERSION}")
include_directories(.)
include_directories(progressbar/include)

# Build libscsicmd for diskscan
include_directories(libscsicmd/include)
add_subdirectory(libscsicmd/src)

# Build diskscan library
add_library(diskscanlib STATIC lib/data.c lib/diskscan.c lib/sha1.c lib/system_id.c lib/verbose.c lib/disk.c
        hdrhistogram/src/hdr_histogram.c hdrhistogram/src/hdr_histogram_log.c
        hdrhistogram/src/hdr_encoding.c ${ARCH_SRC} ${CMAKE_CURRENT_SOURCE_DIR}/include/arch-internal.h)
add_dependencies(diskscanlib scsicmd)

# Build diskscan cli command
add_executable(diskscan diskscan.c cli/cli.c cli/verbose.c progressbar/lib/progressbar.c)
target_link_libraries(diskscan diskscanlib scsicmd m ${tinfo_LIBRARY} ${ZLIB_LIBRARIES} ${LIBS})

install(TARGETS diskscan
        RUNTIME DESTINATION bin)

configure_file(Documentation/diskscan.1.in Documentation/diskscan.1)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/Documentation/diskscan.1
        DESTINATION share/man/man1 COMPONENT doc)
