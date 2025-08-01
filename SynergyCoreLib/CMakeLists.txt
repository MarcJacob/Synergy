add_library(SynergyCoreLib SHARED Sources/SynergyCore.cpp )
target_include_directories(SynergyCoreLib PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Includes/)
target_include_directories(SynergyCoreLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Includes/Public/)
