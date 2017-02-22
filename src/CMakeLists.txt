
set(QT5PROMISE_SOURCES
	Deferred.cpp
	Promise.cpp
	ChildDeferred.cpp
	NetworkDeferred.cpp
)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_library(qt5promise ${QT5PROMISE_SOURCES})
target_link_libraries(qt5promise Qt5::Network)
