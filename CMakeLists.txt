cmake_minimum_required(VERSION 3.5)

set(CMAKE_BUILD_TYPE Debug)

find_package(glfw3 REQUIRED)

project(vulkanproject)

add_executable(vulkanproject main.cpp src/loadBinFile.cpp src/app.cpp src/window.cpp)

include_directories(vulkanproject ${GLFW3_INCLUDE_DIRS})
target_link_libraries(vulkanproject glfw vulkan)

install(TARGETS vulkanproject RUNTIME DESTINATION bin)
