
include_directories(${PROJECT_SOURCE_DIR}/src)
add_executable(test_NetworkPromise NetworkPromiseTest.cpp)
target_link_libraries(test_NetworkPromise Qt5::Core Qt5::Network Qt5::Test)

add_test(NAME NetworkPromise COMMAND test_NetworkPromise)