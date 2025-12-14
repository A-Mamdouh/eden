add_executable(EdenTests
    smoke_test.cpp
)

target_link_libraries(
    EdenTests
    PRIVATE
        Eden
        GTest::gtest_main
)

add_test(NAME EdenTests COMMAND EdenTests)
