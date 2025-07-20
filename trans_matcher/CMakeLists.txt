cmake_minimum_required(VERSION 3.28)
project(trans_matcher)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_PREFIX_PATH "D:/Libs/Qt/Qt-6.6.2/lib/cmake/Qt6")
set(CMAKE_PREFIX_PATH "D:/Libs/Qt/Qt-6.5.2/lib/cmake/Qt6")
find_package(Qt6 REQUIRED COMPONENTS
        Core
        Widgets
        Concurrent
        Network
)
qt_standard_project_setup()
qt6_add_resources(RESOURCES "Resources/Resources.qrc")

include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(trans_matcher
        main.cpp
        core/IPC.cpp
        widgets/TransMatcher.cpp
)
target_link_libraries(trans_matcher PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::Concurrent
        Qt6::Network
)
set_target_properties(trans_matcher PROPERTIES
        WIN32_EXECUTABLE OFF
)