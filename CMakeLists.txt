cmake_minimum_required(VERSION 2.8)
project("LIRISVISION")

# add all sub-projects

add_subdirectory("./BinarizeWolfJolion" "../BinarizeWolfJolion/build")
add_subdirectory("./BodyPartsSegmentation" "../BodyPartsSegmentation/build")
add_subdirectory("./PixeltrackSD" "../PixeltrackSD/build")
add_subdirectory("./ActiveContour" "../ActiveContour/build")

# add global uninstall target

addUninstallTarget()
