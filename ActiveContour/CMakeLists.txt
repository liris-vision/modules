cmake_minimum_required(VERSION 2.8)
project("ActiveContour")

#---------- common settings --------------

include(${CMAKE_CURRENT_SOURCE_DIR}/../CMake_common/common_settings.cmake)

#---------- project specific settings --------------

# source files

FILE(GLOB_RECURSE LIB_SOURCES "src/*.cpp")
    
# includes and libraries

setupOpenCVIncludesAndLibs()

printIncludesAndLIbs()

# build library

addSharedLibrary(activecontour "${LIB_SOURCES}" "${LIBS}")

# build executables

# install configuration files for Starling

installStarlingModule(active_contour.xml app_data/blocks.extra)
installStarlingModule(32-Active-contour-segmentation.hrp app_data/examples.extra)
installStarlingModule(83_active_contour.hrp test.extra)
installStarlingModule(83_output.png.ref test.extra)
addUninstallTarget()




