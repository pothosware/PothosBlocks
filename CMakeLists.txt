########################################################################
# Project setup
########################################################################
cmake_minimum_required(VERSION 2.8.9)
project(PothosBlocks CXX)

if(${CMAKE_PROJECT_NAME} STREQUAL ${PROJECT_NAME})
    find_package(Pothos "0.7.0" CONFIG REQUIRED)
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
# json.hpp header
########################################################################
find_path(JSON_HPP_INCLUDE_DIR NAMES json.hpp PATH_SUFFIXES nlohmann)

if (NOT JSON_HPP_INCLUDE_DIR)
    message(WARNING "Pothos Blocks toolkit requires json.hpp, skipping...")
endif (NOT JSON_HPP_INCLUDE_DIR)

########################################################################
# SIMD dynamic dispatch, if supported by this Pothos build
########################################################################
include(PothosConfigSIMD OPTIONAL RESULT_VARIABLE SIMD_SUPPORTED)
if(SIMD_SUPPORTED)
    include(CheckIncludeFileCXX)
    set(CMAKE_REQUIRED_INCLUDES ${Pothos_INCLUDE_DIRS})
    check_include_file_cxx(Pothos/Util/XSIMDTraits.hpp HAS_POTHOS_XSIMD_TRAITS_HPP)

    if(HAS_POTHOS_XSIMD_TRAITS_HPP)
        find_package(xsimd)
        if(xsimd_FOUND)
            add_definitions(-DPOTHOS_XSIMD)
        endif()
    endif()
endif()

########################################################################
# Build subdirectories
########################################################################
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(file)
add_subdirectory(event)
add_subdirectory(network)
add_subdirectory(packet)
add_subdirectory(serialize)
add_subdirectory(stream)
add_subdirectory(testers)
