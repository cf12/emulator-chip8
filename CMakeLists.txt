cmake_minimum_required(VERSION 3.0.0)
project(EmulatorChip8 VERSION 0.1.0)
set(CMAKE_CXX_STANDARD 17)


# include(CTest)
# enable_testing()


find_package(SDL2 REQUIRED)
add_executable(EmulatorChip8 src/main.cpp src/Chip8.cpp src/Chip8.h src/Platform.cpp src/Platform.h)

target_link_libraries(${PROJECT_NAME} ${SDL2_LIBRARIES})
include_directories(${SDL2_INCLUDE_DIRS})

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)


