add_executable(EdenDemo
    main.cpp
    scripts/PlayerController.cpp
    scenes/demo.cpp
)

target_link_libraries(EdenDemo PRIVATE Eden)
target_compile_features(EdenDemo PRIVATE cxx_std_20)

add_dependencies(EdenDemo EdenShaders)
