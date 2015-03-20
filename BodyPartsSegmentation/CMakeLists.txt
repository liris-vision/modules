cmake_minimum_required(VERSION 2.8)
project("BodyPartsSegmentation")

#---------- common settings --------------

include(${CMAKE_CURRENT_SOURCE_DIR}/../CMake_common/common_settings.cmake)

#---------- project specific settings --------------

# source files

FILE(GLOB_RECURSE LIB_SOURCES "src/*.cpp")
    
# includes and libraries

setupOpenCVIncludesAndLibs()

printIncludesAndLIbs()

# build bodypartssegmentation library

addSharedLibrary(bodypartssegmentation "${LIB_SOURCES}" "${LIBS}")

# build executables

# install configuration files for Starling

installStarlingModule(body_parts_segmentation.xml app_data/blocks.extra)
installStarlingModule(33-Body-parts-segmentation.hrp app_data/examples.extra)
installStarlingModule(77_body_parts_segmentation.hrp test.extra)
installStarlingModule(77_output.png.ref test.extra)
addUninstallTarget()
