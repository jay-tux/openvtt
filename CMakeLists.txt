cmake_minimum_required(VERSION 3.30)
set(CMAKE_PROJECT_TOP_LEVEL_INCLUDES conan_provider.cmake)
project(openvtt)

find_package(Doxygen)

find_package(opengl_system REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glad REQUIRED)
find_package(imgui REQUIRED)
find_package(glm REQUIRED)
find_package(whereami REQUIRED)
find_package(assimp REQUIRED)
find_package(stb REQUIRED)

set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
set(OPENVTT_USE_STACK_TRACE OFF CACHE BOOL "Use C++23 stack trace library")

set(CMAKE_CXX_STANDARD 23)

add_executable(openvtt
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

target_include_directories(openvtt PRIVATE ${CMAKE_SOURCE_DIR})

target_link_libraries(openvtt PRIVATE
        imgui::imgui
        glm::glm
        opengl::opengl
        glfw
        glad::glad
        whereami::whereami
        assimp::assimp
        stb::stb
)

if (OPENVTT_USE_STACK_TRACE)
    target_link_libraries(openvtt PRIVATE stdc++exp)
    target_compile_definitions(openvtt PRIVATE OPENVTT_USE_STACK_TRACE)
endif ()

file(CREATE_LINK "${CMAKE_SOURCE_DIR}/assets" "${CMAKE_BINARY_DIR}/assets" COPY_ON_ERROR SYMBOLIC)

if(DOXYGEN_FOUND)
    doxygen_add_docs(
            docs
            "${CMAKE_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}/renderer/"
            ALL
            CONFIG_FILE ${CMAKE_SOURCE_DIR}/Doxyfile
    )
endif ()