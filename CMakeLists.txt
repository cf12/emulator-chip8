cmake_minimum_required(VERSION 3.0.0)
project(EmulatorChip8 VERSION 0.1.0)
set(CMAKE_CXX_STANDARD 17)

# include(CTest)
# enable_testing()

find_package(glfw3 3.3 REQUIRED)
find_package(OpenGL REQUIRED)

set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/lib/imgui)
include_directories(${IMGUI_DIR} ${IMGUI_DIR}/backends)

add_executable(EmulatorChip8 src/main.cpp src/Chip8.cpp src/Chip8.h ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp ${IMGUI_DIR}/imgui.cpp ${IMGUI_DIR}/imgui_draw.cpp ${IMGUI_DIR}/imgui_demo.cpp ${IMGUI_DIR}/imgui_tables.cpp ${IMGUI_DIR}/imgui_widgets.cpp)

target_link_libraries(${PROJECT_NAME} glfw)
target_link_libraries(${PROJECT_NAME} OpenGL::GL)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)


