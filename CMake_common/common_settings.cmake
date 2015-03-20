#
# This file is shared by several modules and projects.
# Do NOT modify it if you don't know the consequences
# of your changes on the other projects !
#

#cmake_minimum_required(VERSION 2.8)

#-------------------- Common settings part 1/2 -----------------------

# define default build as release mode
if( UNIX )
	if( NOT CMAKE_BUILD_TYPE )
		SET(CMAKE_BUILD_TYPE Release)
	endif()
	message(STATUS "* " ${PROJECT_NAME} " BUILD TYPE: " ${CMAKE_BUILD_TYPE})
endif()

message(STATUS "* " ${PROJECT_NAME} " SOURCE DIR: " ${CMAKE_CURRENT_SOURCE_DIR})
message(STATUS "* " ${PROJECT_NAME} " BINARY DIR: " ${CMAKE_CURRENT_BINARY_DIR})

# define DEBUG in debug mode
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG -D_DEBUG")

# windows specific defines
if( WIN32 )
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /DWIN32 /DD_API_WIN32")
endif()

# unset variables in case they where used in an upper project
unset(INC)
unset(LIBS)

# check that STARLING_DIR is defined
set(STARLING_DIR "" CACHE PATH "Starling directory")
if(NOT STARLING_DIR)
	message(WARNING "STARLING_DIR is NOT defined. This will prevent to install the present module into Starling. If you intend to use this module with Starling, re-run CMake with STARLING_DIR set to Starling directory.")
else()
	message(STATUS "* " ${PROJECT_NAME} " STARLING DIR: " ${STARLING_DIR})
endif()


#-------------------- Macros and functions -----------------------

# Set MSVC output directories
# All outputs (libs, static libs, executables) go to the same directory.
function(setMSVCOutputDir outputDir)
	#message("DBG setMSVCOutputDir outputDir=" "${outputDir}")
	if( MSVC )
		foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
			string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
			set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "${outputDir}" PARENT_SCOPE)
			set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "${outputDir}" PARENT_SCOPE)
			set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "${outputDir}" PARENT_SCOPE)
		endforeach()
	endif()
endFunction()

# Setup OpenCV includes and libraries
# Output in LIBS variable
macro(setupOpenCVIncludesAndLibs)
	# set Opencv includes and libs
	find_package(OpenCV REQUIRED)
	include_directories(${OpenCV_INCLUDE_DIRS})
	list(APPEND LIBS ${OpenCV_LIBS})
endmacro()	

#------------------------------------------------------------

# Setup libconfig includes and libraries
# Output in LIBS variable
macro(setupConfigIncludesAndLibs)
	if( UNIX )
		list(APPEND LIBS "config")
	elseif( WIN32 )
		find_path(CONFIG_INCLUDE_DIR "libconfig.h" PATHS "C:/libs/libconfig-1.4.7/lib" DOC "libconfig include dir (contains libconfig.h)")
		include_directories(${CONFIG_INCLUDE_DIR})

		find_library(CONFIG_LIBRARY NAMES libconfig PATHS "C:/libs/libconfig-1.4.7/Release" DOC "libconfig library release dir")
		list(APPEND LIBS ${CONFIG_LIBRARY})
	endif()
endmacro()	

#------------------------------------------------------------

# Print includes and librairies
function(printIncludesAndLIbs)
	#message(STATUS "* " ${PROJECT_NAME} " INC = " "${INC}")
	get_property(inc_dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
	get_property(link_dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY LINK_DIRECTORIES)
	message(STATUS "* " ${PROJECT_NAME} " INCLUDE DIRS = " "${inc_dirs}")
	message(STATUS "* " ${PROJECT_NAME} " LINK DIRS = " "${link_dirs}")
	message(STATUS "* " ${PROJECT_NAME} " LIBS = " "${LIBS}")
endfunction()

#------------------------------------------------------------

# Add a shared library
function(addSharedLibrary target sources libs)
	#message("DBG addSharedLibrary target=" "${target}")
	#message("DBG addSharedLibrary sources=" "${sources}")
	#message("DBG addSharedLibrary libs=" "${libs}")
	add_library("${target}" SHARED ${sources})
	target_link_libraries("${target}" ${libs})
	if( WIN32 )
		set_target_properties("${target}" PROPERTIES DEFINE_SYMBOL D_BUILDWINDLL)
	endif()
endfunction()

#------------------------------------------------------------

# Add an executable
function(addExecutable target sources libs)
	#message("DBG addExecutable target=" "${target}")
	#message("DBG addExecutable sources=" "${sources}")
	#message("DBG addExecutable libs=" "${libs}")
	add_executable("${target}" ${sources})
	target_link_libraries("${target}" ${libs})
endfunction()

#------------------------------------------------------------

# Get OS description string
function(getOsDescription result)
	if( UNIX )
		execute_process(COMMAND "lsb_release" "-d" OUTPUT_VARIABLE description)
		# for example, returns 'Description:	Ubuntu 12.04.4 LTS'
	else()
		set(description "Windows") 
	endif()
	set("${result}" "${description}" PARENT_SCOPE)	
endfunction()

#------------------------------------------------------------

# Add an 'uninstall' target that reverse the 'install' one
# Ref: http://www.cmake.org/Wiki/CMake_FAQ#Can_I_do_.22make_uninstall.22_with_CMake.3F
# only if we are in the top level project
function(addUninstallTarget)
	if( "${CMAKE_CURRENT_SOURCE_DIR}" STREQUAL "${CMAKE_SOURCE_DIR}" )
		# the place where the uninstall pattern file is
		# depends if we are in a module or in the global modules dir
		set(filepath "${CMAKE_CURRENT_SOURCE_DIR}/CMake_common")
		if(NOT EXISTS "${filepath}/cmake_uninstall.cmake.in")
			set(filepath "${CMAKE_CURRENT_SOURCE_DIR}/../CMake_common")
		endif()
		configure_file("${filepath}/cmake_uninstall.cmake.in" "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake" IMMEDIATE @ONLY)
		add_custom_target(uninstall COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
	endif()
endfunction()

#------------------------------------------------------------

# install a file in Starling directory
# if the file name ends with ".in", @_MODULE_DIR_@ is replaced by its value
function(installStarlingModule filename destination)
	set(filepath "${CMAKE_CURRENT_SOURCE_DIR}/resource")
	# if the file 'filename.in' exists, it is used as a pattern
	if(EXISTS "${filepath}/${filename}.in")
		# replace '@_MODULE_DIR_@' inside the file
		set(_MODULE_DIR_ "${CMAKE_CURRENT_SOURCE_DIR}")
		configure_file("${filepath}/${filename}.in" "${filename}" @ONLY)
	else()
		# copy the file without change
		configure_file("${filepath}/${filename}" "${filename}" COPYONLY)
	endif()

	install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${filename}" DESTINATION "${STARLING_DIR}/${destination}")
endfunction()

#-------------------- Common settings part 2/2 -----------------------

# set executables and libraries output directory
if( ALL_BUILD_IN_MAIN_PROJECT )
	if( UNIX )
		set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
		set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
		set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
	elseif( WIN32 )
		setMSVCOutputDir("${CMAKE_BINARY_DIR}")
	endif()
else()
	# nothing to do on LINUX
	if( WIN32 )
		setMSVCOutputDir("${CMAKE_CURRENT_BINARY_DIR}")
	endif()
endif()
