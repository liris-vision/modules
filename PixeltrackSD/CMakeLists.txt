cmake_minimum_required(VERSION 2.8)
project("PixelTracker")

#---------- common settings --------------

include(${CMAKE_CURRENT_SOURCE_DIR}/../CMake_common/common_settings.cmake)

#---------- project specific settings --------------

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__STDC_CONSTANT_MACROS")
if( UNIX )
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive")
endif()

# source files

#FILE(GLOB_RECURSE SOURCES "../src/*.cpp")
SET(LIB_SOURCES
    src/BGR2HSVdistLUT.cpp
    src/BGR2HSVhistLUT.cpp
    src/Error.cpp
    src/GradDispLUT.cpp
    src/Histogram.cpp
    src/HSVPixelGradientModel.cpp
    src/Output.cpp
    src/OutputTXTFile.cpp
    src/OutputXMLFile.cpp
    src/PixelClassColourModel.cpp
    src/PixelTracker.cpp
    src/Rectangle.cpp
    )
    
SET(EXEC_SOURCES
    example/pixeltrack.cpp
    src/Draw.cpp
    src/ImageOutput.cpp
    src/Timer.cpp
    src/util.cpp
    src/utils_cv.cpp
    src/VideoInput.cpp
    src/VideoInputFile.cpp
    src/VideoOutput.cpp
    )

# includes and libraries

setupOpenCVIncludesAndLibs()

printIncludesAndLIbs()

# build pixeltracker library

addSharedLibrary(pixeltracker "${LIB_SOURCES}" "${LIBS}")

# build executables

if( NOT WIN32 )
    addExecutable(pixeltrack "${EXEC_SOURCES}" pixeltracker)
endif()

# install configuration files for Starling

installStarlingModule(pixel_track_sd.xml app_data/blocks.extra)
installStarlingModule(31-Pixel-track-SD.hrp app_data/examples.extra)
installStarlingModule(82_pixel_track_sd.hrp test.extra)
installStarlingModule(82_output.mpeg.ref.u1404-64 test.extra)
installStarlingModule(82_output.mpeg.ref.u1604-64 test.extra)
installStarlingModule(82_output.mpeg.ref.w7 test.extra)
addUninstallTarget()

