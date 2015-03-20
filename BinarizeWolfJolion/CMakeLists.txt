cmake_minimum_required(VERSION 2.8)
project("BinarizeWolfJolion")

#---------- common settings --------------

include(${CMAKE_CURRENT_SOURCE_DIR}/../CMake_common/common_settings.cmake)

#---------- project specific settings --------------

# source files

FILE(GLOB_RECURSE LIB_SOURCES "src/*.cpp")

# includes and libraries

setupOpenCVIncludesAndLibs()

printIncludesAndLIbs()

# build binarizewolfjolion library

addSharedLibrary(binarizewolfjolion "${LIB_SOURCES}" "${LIBS}")

# build executables

# install configuration files for Starling

installStarlingModule(binarize_wolf_jolion.xml app_data/blocks.extra)
installStarlingModule(30-Binarize-Wolf-Jolion.hrp app_data/examples.extra)
installStarlingModule(70_binarize_wolf_jolion.hrp test.extra)
installStarlingModule(70_output.png.ref test.extra)
addUninstallTarget()

