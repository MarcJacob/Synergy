add_executable(SynergyServer Sources/SynergyServerMain.cpp )
target_include_directories(SynergyServer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../SynergyCoreLib/Sources/Public/)
link_libraries(SynergyServer SynergyCoreLib)