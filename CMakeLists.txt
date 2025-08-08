cmake_minimum_required(VERSION 3.10)

project(Synergy VERSION 0.1)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/SynergyClientLib/)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/SynergyServer/)