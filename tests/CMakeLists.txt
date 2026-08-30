add_executable(EdenTests
    EventServiceTest.cpp
    ClockServiceTest.cpp
    SceneTest.cpp
    TransformSystemTest.cpp
    ScriptSystemTest.cpp
    InputSystemTest.cpp
    NullRendererTest.cpp
    RenderSystemMaterialTest.cpp
    BoundsTest.cpp
    FrustumTest.cpp
)

target_link_libraries(
    EdenTests
    PRIVATE
        Eden::Eden
        GTest::gtest_main
)

add_test(NAME EdenTests COMMAND EdenTests)
