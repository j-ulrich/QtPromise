cmake_minimum_required(VERSION 3.0.0)

project(QtPromise)
set(PROJECT_VERSION 0.0.1)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt5 COMPONENTS Core Network REQUIRED)

get_target_property(QT5CORE_LOCATION Qt5::Core LOCATION)
get_filename_component(QT5_BIN_DIR ${QT5CORE_LOCATION} DIRECTORY)
set(CMAKE_MSVCIDE_RUN_PATH ${QT5_BIN_DIR})


add_subdirectory(src)
add_subdirectory(doc)
enable_testing()
add_subdirectory(tests)