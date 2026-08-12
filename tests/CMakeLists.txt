include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.15.2
)
# Keep gtest's own runtime settings from clobbering the parent project's.
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(dzsungel-tests
    WavetableStoreTest.cpp
    ChannelStateTest.cpp
    VoiceAllocatorTest.cpp
)

target_link_libraries(dzsungel-tests PRIVATE dzsungel-core gtest_main)

include(GoogleTest)
gtest_discover_tests(dzsungel-tests)
