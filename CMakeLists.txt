cmake_minimum_required(VERSION 2.8)
project(file_watcher)

add_definitions("-Wall -Wextra -pedantic")
find_package(PkgConfig)

pkg_check_modules(GLIB REQUIRED glib-2.0)
pkg_check_modules(GIO REQUIRED gio-2.0)

include_directories(${GIO_INCLUDE_DIRS} ${GLIB_INCLUDE_DIRS})
link_directories(${GIO_LIBRARY_DIRS} ${GLIB_LIBRARY_DIRS})
add_executable(file_watcher src/main.c src/file_watcher.c)
target_link_libraries(file_watcher ${GLIB_LIBRARIES} ${GIO_LIBRARIES})