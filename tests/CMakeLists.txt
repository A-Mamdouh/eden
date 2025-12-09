add_executable(EdenTests
    EngineArchitectureTests.cpp
    InputTests.cpp
)

target_link_libraries(
    EdenTests
    PRIVATE
        Eden
        GTest::gtest_main
)

add_test(NAME EdenTests COMMAND EdenTests)
