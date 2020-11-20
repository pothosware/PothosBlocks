########################################################################
## Feature registration
########################################################################
cmake_dependent_option(ENABLE_BLOCKS_FILE "Enable Pothos Blocks.File component" ON "ENABLE_BLOCKS;JSON_HPP_INCLUDE_DIR" OFF)
add_feature_info("  File" ENABLE_BLOCKS_FILE "Filesystem resource source/sink blocks")
if (NOT ENABLE_BLOCKS_FILE)
    return()
endif()

########################################################################
# File blocks module
########################################################################
set(fileBlocksSources
    BinaryFileSink.cpp
    BinaryFileSource.cpp
    MemoryMappedBufferContainer.cpp
    MemoryMappedBufferManager.cpp
    TextFileSink.cpp
    TestBinaryFileBlocks.cpp
)

include_directories(${JSON_HPP_INCLUDE_DIR})
POTHOS_MODULE_UTIL(
    TARGET FileBlocks
    SOURCES ${fileBlocksSources}
    DESTINATION blocks
    ENABLE_DOCS
)
