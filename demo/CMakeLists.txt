add_executable(Eden_Demo
    main.cpp
    scripts/PlayerController.cpp
    scenes/demo.cpp
)

target_link_libraries(Eden_Demo PRIVATE Eden)
target_compile_features(Eden_Demo PRIVATE cxx_std_20)

add_dependencies(Eden_Demo EdenShaders)
