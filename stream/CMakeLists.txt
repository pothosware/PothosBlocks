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
include_directories(${JSON_HPP_INCLUDE_DIR})
POTHOS_MODULE_UTIL(
    TARGET StreamBlocks
    SOURCES
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
    DESTINATION blocks
    ENABLE_DOCS
)
