add_library(SynergyClientLib SHARED Sources/SynergyClientMain.cpp )
target_include_directories(SynergyClientLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Sources/Public/)
link_libraries(SynergyClientLib SynergyCoreLib)