cmake_minimum_required(VERSION 3.30)
set(CMAKE_PROJECT_TOP_LEVEL_INCLUDES conan_provider.cmake)
project(gltt)

find_package(opengl_system)
find_package(glfw3)
find_package(glad)
find_package(imgui)
find_package(glm)
find_package(whereami)

set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

set(CMAKE_CXX_STANDARD 20)

add_executable(gltt
        # explicit dependencies (ImGui)
        bindings/imgui_impl_glfw.cpp
        bindings/imgui_impl_opengl3.cpp

        # source files
        main.cpp
        renderer/window.cpp
        renderer/fps_counter.cpp
        renderer/log_view.cpp
)

target_include_directories(gltt PRIVATE ${CMAKE_SOURCE_DIR})

target_link_libraries(gltt PRIVATE
        imgui::imgui
        glm::glm
        opengl::opengl
        glfw
        glad::glad
        whereami::whereami
)

file(CREATE_LINK "${CMAKE_SOURCE_DIR}/assets" "${CMAKE_BINARY_DIR}/assets" COPY_ON_ERROR SYMBOLIC)
