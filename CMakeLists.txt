cmake_minimum_required(VERSION 3.0.0)

project(QtPromise)
set(PROJECT_VERSION 0.0.1)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt5Core REQUIRED)
find_package(Qt5Network REQUIRED)


add_subdirectory(src)
enable_testing()
add_subdirectory(tests)