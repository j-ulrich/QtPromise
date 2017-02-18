
add_executable(NetworkPromiseTest NetworkPromiseTest.cpp)
target_link_libraries(NetworkPromiseTest Qt5::Core Qt5::Network Qt5::Test)

add_test(NAME NetworkPromise COMMAND NetworkPromiseTest)