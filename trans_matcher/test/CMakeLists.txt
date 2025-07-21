add_executable(test
        test.cpp
        ../widgets/TransMatcher.cpp
)
target_link_libraries(test PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::Concurrent
        Qt6::Network
)
set_target_properties(test PROPERTIES
        WIN32_EXECUTABLE OFF
)