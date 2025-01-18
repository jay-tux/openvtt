cmake_minimum_required(VERSION 3.30)
set(CMAKE_PROJECT_TOP_LEVEL_INCLUDES conan_provider.cmake)
project(openvtt)

find_package(Doxygen)

find_package(Java REQUIRED)
find_package(antlr4 REQUIRED)

find_package(opengl_system REQUIRED)
find_package(glfw3 REQUIRED)
find_package(imgui REQUIRED)
find_package(glm REQUIRED)
find_package(whereami REQUIRED)
find_package(assimp REQUIRED)
find_package(stb REQUIRED)
find_package(antlr4-runtime REQUIRED)

set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
set(OPENVTT_USE_STACK_TRACE OFF CACHE BOOL "Use C++23 stack trace library")

set(CMAKE_CXX_STANDARD 23)

if(DEFINED openjdk_BIN_DIRS_DEBUG)
    set(JAVA_EXE ${openjdk_BIN_DIRS_DEBUG}/java)
elseif(DEFINED openjdk_BIN_DIRS_RELEASE)
    set(JAVA_EXE ${openjdk_BIN_DIRS_RELEASE}/java)
else()
    message(FATAL_ERROR "Could not find Java executable")
endif()

if(DEFINED antlr4_BIN_DIRS_DEBUG)
    set(ANTLR_EXE ${antlr4_BIN_DIRS_DEBUG}/antlr4)
    set(ANTLR_LIB ${antlr4_PACKAGE_FOLDER_DEBUG}/res/antlr-complete.jar)
elseif(DEFINED antlr4_BIN_DIRS_RELEASE)
    set(ANTLR_EXE ${antlr4_BIN_DIRS_RELEASE}/antlr4)
    set(ANTLR_LIB ${antlr4_PACKAGE_FOLDER_RELEASE}/res/antlr-complete.jar)
else()
    message(FATAL_ERROR "Could not find ANTLR executable/JAR")
endif()

message(STATUS "JAVA_EXE=${JAVA_EXE} // ANTLR_EXECUTABLE=${ANTLR_EXE}")

function(antlr_target TARGET GRAMMAR PACKAGE OUT_DIR)
    get_filename_component(${GRAMMAR}_SRC ${GRAMMAR} NAME_WLE)
    set(${TARGET}_SOURCES
            ${OUT_DIR}/${${GRAMMAR}_SRC}Lexer.cpp ${OUT_DIR}/${${GRAMMAR}_SRC}Parser.cpp
            ${OUT_DIR}/${${GRAMMAR}_SRC}BaseVisitor.cpp ${OUT_DIR}/${${GRAMMAR}_SRC}Visitor.cpp
    )
    add_custom_command(
            OUTPUT ${${TARGET}_SOURCES}
            COMMAND ${JAVA_EXE} -classpath ${ANTLR_LIB} org.antlr.v4.Tool -o ${OUT_DIR} -Dlanguage=Cpp -no-listener -visitor -package ${PACKAGE} ${GRAMMAR}
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            DEPENDS ${GRAMMAR}
    )

    add_library(${TARGET} STATIC ${${TARGET}_SOURCES})
    target_link_libraries(${TARGET} PUBLIC antlr4_static)
    target_include_directories(${TARGET} PUBLIC ${OUT_DIR})
endfunction()

antlr_target(map_spec ${CMAKE_SOURCE_DIR}/grammars/map.g4 openvtt::map ${CMAKE_BINARY_DIR}/antlr_out/map)

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
        renderer/collider.cpp
        renderer/collider.hpp
        renderer/fbo.cpp
        renderer/glad.cpp
        map/map_parser.cpp
        map/map_visitor.cpp
        map/object_cache.cpp
        renderer/gizmos.cpp
)

target_compile_definitions(openvtt PRIVATE
        $<$<CONFIG:Debug>:OPENVTT_DEBUG>
        $<$<CONFIG:Release>:OPENVTT_RELEASE>
)

target_include_directories(openvtt PRIVATE ${CMAKE_SOURCE_DIR})

target_link_libraries(openvtt PRIVATE
        imgui::imgui
        glm::glm
        opengl::opengl
        glfw
        whereami::whereami
        assimp::assimp
        stb::stb
        antlr4_static
        map_spec
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Debug build - using stack trace library")
    target_link_libraries(openvtt PRIVATE stdc++exp)
    if (OPENVTT_USE_STACK_TRACE)
        target_compile_definitions(openvtt PRIVATE OPENVTT_USE_STACK_TRACE)
    endif ()
else()
    message(STATUS "Release build - no stack trace library")
endif()

file(CREATE_LINK "${CMAKE_SOURCE_DIR}/assets" "${CMAKE_BINARY_DIR}/assets" COPY_ON_ERROR SYMBOLIC)

if(DOXYGEN_FOUND)
    doxygen_add_docs(
            docs
            "${CMAKE_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}/renderer/" "${CMAKE_SOURCE_DIR}/map/"
            ALL
            CONFIG_FILE ${CMAKE_SOURCE_DIR}/Doxyfile
    )
endif ()