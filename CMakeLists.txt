cmake_minimum_required(VERSION 2.8)
project(file_watcher)

option(DEBUG_ON "Turn on the debug" ON)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" OR
    "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(warnings "-Wall -Wextra -Werror")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    set(warnings "/W4 /WX /EHsc")
endif()
find_package(PkgConfig)

if (DEBUG_ON)
	set(CMAKE_C_FLAGS "-ggdb -O0")
endif(DEBUG_ON)

pkg_check_modules(GLIB REQUIRED glib-2.0)
pkg_check_modules(GIO REQUIRED gio-2.0)
pkg_check_modules(GITLIB REQUIRED libgit2)

include_directories(${GIO_INCLUDE_DIRS} ${GLIB_INCLUDE_DIRS} ${GITLIB_INCLUDE_DIRS})
link_directories(${GIO_LIBRARY_DIRS} ${GLIB_LIBRARY_DIRS} ${GITLIB_LIBRARY_DIRS})

add_executable(file_watcher src/main.c src/file_watcher.c
	src/file_keeper.c src/utils.c src/file_conn.c
	src/file_message.c)

target_link_libraries(file_watcher ${GLIB_LIBRARIES} ${GIO_LIBRARIES} ${GITLIB_LIBRARIES})