########################################################################
## Feature registration
########################################################################
cmake_dependent_option(ENABLE_BLOCKS_STREAM "Enable Pothos Blocks.Stream component" ON "ENABLE_BLOCKS;JSON_HPP_INCLUDE_DIR" OFF)
add_feature_info("  Stream" ENABLE_BLOCKS_STREAM "Miscellaneous stream manipulation blocks")
if (NOT ENABLE_BLOCKS_STREAM)
    return()
endif()

########################################################################
# Stream blocks module
########################################################################
set(sources
    Converter.cpp
    TestConverter.cpp
    Copier.cpp
    Delay.cpp
    TestDelay.cpp
    DynamicRouter.cpp
    Pacer.cpp
    Relabeler.cpp
    LabelStripper.cpp
    Gateway.cpp
    TestGateway.cpp
    Reinterpret.cpp
    RateMonitor.cpp
    MinMax.cpp
    TestMinMax.cpp
    Interleaver.cpp
    TestInterleaver.cpp
    Deinterleaver.cpp
    Clamp.cpp
    TestClamp.cpp
    Repeat.cpp
    TestRepeat.cpp
    IsX.cpp
    TestIsX.cpp
    Mute.cpp
    Round.cpp
    TestRoundBlocks.cpp
    Replace.cpp
    TestReplace.cpp
    FirstN.cpp
    TestFirstN.cpp)
set(libraries "")

if(xsimd_FOUND)
    set(SIMDInputs
        SIMD/Clamp.cpp
        SIMD/IsX.cpp
        SIMD/MinMax.cpp
        SIMD/Round.cpp)

    PothosGenerateSIMDSources(
        SIMDSources
        SIMD/StreamBlocks.json
        ${SIMDInputs})

    list(APPEND sources ${SIMDSources})
    include_directories(${CMAKE_CURRENT_BINARY_DIR})
    list(APPEND libraries xsimd)
endif()

include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${JSON_HPP_INCLUDE_DIR})
POTHOS_MODULE_UTIL(
    TARGET StreamBlocks
    SOURCES ${sources}
    LIBRARIES ${libraries}
    DESTINATION blocks
    ENABLE_DOCS
)
if(xsimd_FOUND)
    add_dependencies(StreamBlocks StreamBlocks_SIMDDispatcher)
endif()
