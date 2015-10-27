cmake_minimum_required(VERSION 2.8)
project(libscsicmd)

export(PACKAGE libscsicmd)

include_directories("include")
add_compile_options(-Wall -Wextra -Wshadow -Wmissing-prototypes -Winit-self)
add_definitions(-D_GNU_SOURCE -D_FORTIFY_SOURCE=2)

add_subdirectory(src)
add_subdirectory(test)
