########################################################################
# Project setup
########################################################################
cmake_minimum_required(VERSION 2.8.9)
project(PothosBlocks CXX)

if(${CMAKE_PROJECT_NAME} STREQUAL ${PROJECT_NAME})
    find_package(Pothos "0.4.0" CONFIG REQUIRED)
else()
    find_package(Pothos CONFIG REQUIRED) #in-tree build
endif()

########################################################################
## Feature registration
########################################################################
include(FeatureSummary)
include(CMakeDependentOption)
cmake_dependent_option(ENABLE_BLOCKS "Enable Pothos Blocks component" ON "Pothos_FOUND" OFF)
add_feature_info(Blocks ENABLE_BLOCKS "A collection of general purpose blocks")

########################################################################
# Build subdirectories
########################################################################
add_subdirectory(file)
add_subdirectory(event)
add_subdirectory(network)
add_subdirectory(packet)
add_subdirectory(serialize)
add_subdirectory(stream)
add_subdirectory(testers)
