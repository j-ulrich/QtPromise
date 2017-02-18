
add_executable(PromiseTest PromiseTest.cpp)
target_link_libraries(PromiseTest Qt5::Core Qt5::Test)

add_test(NAME Promise COMMAND PromiseTest)