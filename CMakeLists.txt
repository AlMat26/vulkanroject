cmake_minimum_required(VERSION 3.0)

project(vulkanproject)

add_executable(vulkanproject main.cpp)

install(TARGETS vulkanproject RUNTIME DESTINATION bin)
