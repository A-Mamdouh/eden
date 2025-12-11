add_executable(Eden_Demo
    main.cpp
    scenes/demo.cpp
)

target_include_directories(Eden_Demo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Eden_Demo PRIVATE Eden spdlog::spdlog)
target_compile_features(Eden_Demo PRIVATE cxx_std_20)

add_dependencies(Eden_Demo EdenShaders)
