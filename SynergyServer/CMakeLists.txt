add_executable(SynergyServer Sources/SynergyServerMain.cpp )
target_include_directories(SynergyServer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Includes/)
target_include_directories(SynergyServer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../SynergyCoreLib/Includes/Public/)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../SynergyCoreLib Build)
link_libraries(SynergyServer SynergyCoreLib)