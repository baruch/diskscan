cmake_minimum_required(VERSION 2.8)
project(libscsicmd)

export(PACKAGE libscsicmd)

include_directories("include")
add_compile_options(-Wall -Wextra -Wshadow -Wmissing-prototypes -Winit-self -g)
add_definitions(-D_GNU_SOURCE -D_FORTIFY_SOURCE=2)

set(CMAKE_C_FLAGS_DEBUG "-Werror -O0 ${CMAKE_C_FLAGS_DEBUG}")
set(CMAKE_C_FLAGS_RELEASE "-Wall -O3 ${CMAKE_C_FLAGS_RELEASE}")

add_subdirectory(src)
add_subdirectory(test)
