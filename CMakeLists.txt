# Porkchop — build with:
#   cmake -S . -B build -DKFL_DIR=/path/to/KFL && cmake --build build -j
#   ./build/porkchop
#
# Linux only. Depends on the KFL astrodynamics toolkit
# (https://github.com/savannah-i-g/KFL) for the Lambert solver and Kepler
# propagator; point -DKFL_DIR at a built KFL tree. Dear ImGui is vendored.

cmake_minimum_required(VERSION 3.16)
project(Porkchop LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

find_package(OpenGL REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GLFW REQUIRED glfw3)

set(KFL_DIR "/home/federated-industrial/Documents/Garden/Athena/KFL_public"
    CACHE PATH "Root of a built KFL tree (github.com/savannah-i-g/KFL)")

# Vendored Dear ImGui (docking) — see NOTICE.
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui)
add_library(imgui STATIC
    ${IMGUI_DIR}/imgui.cpp ${IMGUI_DIR}/imgui_draw.cpp ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp)
target_include_directories(imgui PUBLIC ${IMGUI_DIR} ${IMGUI_DIR}/backends ${GLFW_INCLUDE_DIRS})

add_executable(porkchop
    src/app/main.cpp
    src/ui/ui.cpp
    src/core/julian.cpp
    src/core/ephemeris.cpp
    src/core/transfer.cpp)

target_include_directories(porkchop PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${KFL_DIR}/libk26astro_conics/include
    ${KFL_DIR}/libk26astro_core/include
    ${KFL_DIR}/libk26astro_body/include
    ${KFL_DIR}/libk26m3d/include
    ${GLFW_INCLUDE_DIRS})
target_link_directories(porkchop PRIVATE ${GLFW_LIBRARY_DIRS})
target_link_libraries(porkchop PRIVATE
    imgui OpenGL::GL ${GLFW_LIBRARIES}
    ${KFL_DIR}/libk26astro_conics/libk26astro_conics.a
    ${KFL_DIR}/libk26astro_body/libk26astro_body.a
    ${KFL_DIR}/libk26astro_core/libk26astro_core.a
    ${KFL_DIR}/libk26m3d/libk26m3d.a)
