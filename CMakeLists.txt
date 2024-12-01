cmake_minimum_required(VERSION 3.30)
set(CMAKE_PROJECT_TOP_LEVEL_INCLUDES conan_provider.cmake)
project(gltt)

find_package(opengl_system)
find_package(glfw3)
find_package(glad)
find_package(imgui)
find_package(glm)
find_package(whereami)
find_package(assimp)
find_package(stb)

set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
set(GLTT_USE_STACK_TRACE OFF CACHE BOOL "Use C++23 stack trace library")

set(CMAKE_CXX_STANDARD 23)

add_executable(gltt
        # explicit dependencies (ImGui)
        bindings/imgui_impl_glfw.cpp
        bindings/imgui_impl_opengl3.cpp

        # source files
        main.cpp
        renderer/window.cpp
        renderer/fps_counter.cpp
        renderer/log_view.cpp
        renderer/object.cpp
        renderer/shader.cpp
        renderer/camera.cpp
        renderer/texture.cpp
        renderer/stb_impl.cpp
        renderer/render_cache.cpp
        renderer/renderable.cpp
)

target_include_directories(gltt PRIVATE ${CMAKE_SOURCE_DIR})

target_link_libraries(gltt PRIVATE
        imgui::imgui
        glm::glm
        opengl::opengl
        glfw
        glad::glad
        whereami::whereami
        assimp::assimp
        stb::stb
)

if (GLTT_USE_STACK_TRACE)
    target_link_libraries(gltt PRIVATE stdc++exp)
    target_compile_definitions(gltt PRIVATE GLTT_USE_STACK_TRACE)
endif ()

file(CREATE_LINK "${CMAKE_SOURCE_DIR}/assets" "${CMAKE_BINARY_DIR}/assets" COPY_ON_ERROR SYMBOLIC)
