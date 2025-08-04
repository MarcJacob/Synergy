add_library(SynergyClientLib SHARED Sources/SynergyClientMain.cpp )
target_include_directories(SynergyClientLib PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Includes/)
target_include_directories(SynergyClientLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Includes/Public/)
target_include_directories(SynergyClientLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../SynergyCoreLib/Includes/Public/)
link_libraries(SynergyClientLib SynergyCoreLib)

target_link_options(SynergyClientLib PRIVATE "/PDBALTPATH:SynergyClientLib.pdb")