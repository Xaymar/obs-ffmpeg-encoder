# FFMPEG Video Encoder Integration for OBS Studio
# Copyright (c) 2019 Michael Fabian Dirks <info@xaymar.com>
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

# CMake Setup
CMake_Minimum_Required(VERSION 3.8.0)
Include("cmake/util.cmake")

# Automatic Versioning
set(VERSION_MAJOR 0)
set(VERSION_MINOR 3)
set(VERSION_PATCH 0)
set(VERSION_TWEAK 0)
set(PROJECT_COMMIT "N/A")
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/.git")
	set(GIT_RESULT "")
	set(GIT_OUTPUT "")
	execute_process(
		COMMAND git rev-list --count --topo-order ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}..HEAD
		WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
		RESULT_VARIABLE GIT_RESULT
		OUTPUT_VARIABLE GIT_OUTPUT
		OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_STRIP_TRAILING_WHITESPACE ERROR_QUIET
	)
	if(GIT_RESULT EQUAL 0)
		set(VERSION_TWEAK ${GIT_OUTPUT})
	endif()
	execute_process(
		COMMAND git rev-parse HEAD
		WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
		RESULT_VARIABLE GIT_RESULT
		OUTPUT_VARIABLE GIT_OUTPUT
		OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_STRIP_TRAILING_WHITESPACE ERROR_QUIET
	)
	if(GIT_RESULT EQUAL 0)
		set(PROJECT_COMMIT ${GIT_OUTPUT})
	endif()
endif()

# All Warnings, Extra Warnings, Pedantic
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
	# using Clang
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-missing-braces -Wmissing-field-initializers -Wno-c++98-compat-pedantic -Wold-style-cast -Wno-documentation -Wno-documentation-unknown-command -Wno-covered-switch-default -Wno-switch-enum")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
	# GCC: -fpermissive is required as GCC does not allow the same template to be in different namespaces.
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wpedantic -fpermissive -Wno-long-long -Wno-missing-braces -Wmissing-field-initializers")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
	# using Intel C++
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
	# Force to always compile with W4
	if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
		string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	else()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
	endif()
endif()
# C++ Standard and Extensions
## Use C++17 and no non-standard extensions.
set(_CXX_STANDARD 17)
set(_CXX_EXTENSIONS OFF)

# Define Project
project(
	obs-ffmpeg-encoder
	VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}.${VERSION_TWEAK}
)
set(PROJECT_FULL_NAME "FFMPEG Encoder for OBS Studio")
set(PROJECT_DESCRIPTION "FFmpeg Encoders for OBS Studio")
set(PROJECT_AUTHORS "Michael Fabian 'Xaymar' Dirks <info@xaymar.com>")
set(PROJECT_COPYRIGHT_YEARS "2018 - 2019")

################################################################################
# Setup / Bootstrap
################################################################################

# Detect Build Type
if("${CMAKE_SOURCE_DIR}" STREQUAL "${PROJECT_SOURCE_DIR}")
	set(PropertyPrefix "")
else()
	set(PropertyPrefix "${PROJECT_NAME}_")
endif()

# Detect Architecture
math(EXPR BITS "8*${CMAKE_SIZEOF_VOID_P}")
if("${BITS}" STREQUAL "32")
	set(ARCH "x86")
else()
	set(ARCH "x64")
endif()

# Search Path
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")

################################################################################
# CMake / Compiler
################################################################################

# Configure Version Header
configure_file(
	"${PROJECT_SOURCE_DIR}/cmake/version.hpp.in"
	"${PROJECT_BINARY_DIR}/source/version.hpp"
	@ONLY
)
configure_file(
	"${PROJECT_SOURCE_DIR}/cmake/module.cpp.in"
	"${PROJECT_BINARY_DIR}/source/module.cpp"
	@ONLY
)

# Windows
if (WIN32)
	## Installer (InnoSetup)
	get_filename_component(ISS_FILES_DIR "${CMAKE_INSTALL_PREFIX}" ABSOLUTE)
	file(TO_NATIVE_PATH "${ISS_FILES_DIR}" ISS_FILES_DIR)
	get_filename_component(ISS_PACKAGE_DIR "${CMAKE_PACKAGE_PREFIX}" ABSOLUTE)
	file(TO_NATIVE_PATH "${ISS_PACKAGE_DIR}" ISS_PACKAGE_DIR)
	get_filename_component(ISS_SOURCE_DIR "${PROJECT_SOURCE_DIR}" ABSOLUTE)
	file(TO_NATIVE_PATH "${ISS_SOURCE_DIR}" ISS_SOURCE_DIR)
	configure_file(
		"${PROJECT_SOURCE_DIR}/cmake/installer.iss.in"
		"${PROJECT_BINARY_DIR}/installer.iss"
		@ONLY
	)

	# Windows Specific Resource Definition
	set(PROJECT_PRODUCT_NAME "${PROJECT_FULL_NAME}")
	set(PROJECT_COMPANY_NAME "${PROJECT_AUTHORS}")
	set(PROJECT_COPYRIGHT "${PROJECT_AUTHORS} © ${PROJECT_COPYRIGHT_YEARS}")
	set(PROJECT_LEGAL_TRADEMARKS_1 "")
	set(PROJECT_LEGAL_TRADEMARKS_2 "")
	
	configure_file(
		"${PROJECT_SOURCE_DIR}/cmake/version.rc.in"
		"${PROJECT_BINARY_DIR}/cmake/version.rc"
		@ONLY
	)	
endif()

################################################################################
# Options
################################################################################
set(${PropertyPrefix}OBS_NATIVE FALSE CACHE BOOL "Use native obs-studio build" FORCE)
set(${PropertyPrefix}OBS_REFERENCE FALSE CACHE BOOL "Use referenced obs-studio build" FORCE)
set(${PropertyPrefix}OBS_PACKAGE FALSE CACHE BOOL "Use packaged obs-studio build" FORCE)
set(${PropertyPrefix}OBS_DOWNLOAD FALSE CACHE BOOL "Use downloaded obs-studio build" FORCE)
mark_as_advanced(FORCE OBS_NATIVE OBS_PACKAGE OBS_REFERENCE OBS_DOWNLOAD)

if(NOT TARGET libobs)
	set(${PropertyPrefix}OBS_STUDIO_DIR "" CACHE PATH "OBS Studio Source/Package Directory")
	set(${PropertyPrefix}OBS_DOWNLOAD_VERSION "24.0.0-rc2-ci" CACHE STRING "OBS Studio Version to download")
endif()

if(NOT ${PropertyPrefix}OBS_NATIVE)
	set(${PropertyPrefix}OBS_DEPENDENCIES_DIR "" CACHE PATH "Path to OBS Dependencies")
	set(CMAKE_PACKAGE_PREFIX "${CMAKE_BINARY_DIR}" CACHE PATH "Path for generated archives.")
	set(CMAKE_PACKAGE_NAME "${PROJECT_NAME}" CACHE STRING "Name for the generated archives.")
	set(CMAKE_PACKAGE_SUFFIX_OVERRIDE "" CACHE STRING "Override for the suffix.")
endif()

################################################################################
# Dependencies
################################################################################

# Detect OBS Studio Type
if(TARGET libobs)
	message(STATUS "${PROJECT_NAME}: Using native obs-studio.")
	CacheSet(${PropertyPrefix}OBS_NATIVE TRUE)	
else()
	CacheSet(${PropertyPrefix}OBS_NATIVE FALSE)
	if(EXISTS "${OBS_STUDIO_DIR}/cmake/LibObs/LibObsConfig.cmake")
		message(STATUS "${PROJECT_NAME}: Using packaged obs-studio.")
		CacheSet(${PropertyPrefix}OBS_PACKAGE TRUE)
	elseif(EXISTS "${OBS_STUDIO_DIR}/libobs/obs-module.h")
		message(STATUS "${PROJECT_NAME}: Using referenced obs-studio.")
		CacheSet(${PropertyPrefix}OBS_REFERENCE TRUE)
	else()
		message(STATUS "${PROJECT_NAME}: No OBS Studio detected, using downloadable prebuilt binaries.")
		CacheSet(${PropertyPrefix}OBS_DOWNLOAD TRUE)
		set(${PropertyPrefix}OBS_DOWNLOAD_URL "https://github.com/Xaymar/obs-studio/releases/download/${OBS_DOWNLOAD_VERSION}/obs-studio-${ARCH}-0.0.0.0-vs2017.7z")
	endif()
endif()

# CMake Modules
if(${PropertyPrefix}OBS_DOWNLOAD)
	include("cmake/DownloadProject.cmake")
endif()

# Load OBS Studio
if(${PropertyPrefix}OBS_NATIVE)
elseif(${PropertyPrefix}OBS_PACKAGE)
	include("${OBS_STUDIO_DIR}/cmake/LibObs/LibObsConfig.cmake")
elseif(${PropertyPrefix}OBS_REFERENCE)
	set(obsPath "${OBS_STUDIO_DIR}")
	include("${OBS_STUDIO_DIR}/cmake/external/Findlibobs.cmake")
elseif(${PropertyPrefix}OBS_DOWNLOAD)
	download_project(
		PROJ libobs
		URL ${OBS_DOWNLOAD_URL}
		UPDATE_DISCONNECTED 1
	)
	include("${libobs_SOURCE_DIR}/cmake/LibObs/LibObsConfig.cmake")
else()
	message(CRITICAL "Impossible case reached, verify system stability.")
	return()
endif()

# FFmpeg (compatible with OBS Studio)
if(NOT ${PropertyPrefix}OBS_NATIVE)
	if(WIN32)
		if(NOT IS_DIRECTORY FFmpegPath)
			if(IS_DIRECTORY "${${PropertyPrefix}OBS_DEPENDENCIES_DIR}")
				set(FFmpegPath "${${PropertyPrefix}OBS_DEPENDENCIES_DIR}/win${BITS}/")
			else()
				download_project(
					PROJ obsdeps
					URL "https://obsproject.com/downloads/dependencies2017.zip"
					UPDATE_DISCONNECTED 1
				)
				set(FFmpegPath "${obsdeps_SOURCE_DIR}/win${BITS}/")
			endif()
		endif()
		
		set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} /SAFESEH:NO")
		set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} /SAFESEH:NO")
	endif()
endif()
find_package(FFmpeg REQUIRED COMPONENTS avutil avcodec swscale)

################################################################################
# Code
################################################################################
set(PROJECT_DATA
	"${PROJECT_SOURCE_DIR}/data/locale/en-US.ini"
	"${PROJECT_SOURCE_DIR}/LICENSE"
)

set(PROJECT_LIBRARIES
)

set(PROJECT_TEMPLATES
	"${PROJECT_SOURCE_DIR}/cmake/version.hpp.in"
	"${PROJECT_SOURCE_DIR}/cmake/module.cpp.in"	
)
if(WIN32)
	list(APPEND PROJECT_TEMPLATES
		"${PROJECT_SOURCE_DIR}/cmake/installer.iss.in"
		"${PROJECT_SOURCE_DIR}/cmake/version.rc.in"
	)
endif()

set(PROJECT_GENERATED
	"${PROJECT_BINARY_DIR}/source/module.cpp"
	"${PROJECT_BINARY_DIR}/source/version.hpp"
)
set(PROJECT_PRIVATE
	"${PROJECT_SOURCE_DIR}/source/encoder.hpp"
	"${PROJECT_SOURCE_DIR}/source/encoder.cpp"
	"${PROJECT_SOURCE_DIR}/source/plugin.cpp"
	"${PROJECT_SOURCE_DIR}/source/plugin.hpp"
	"${PROJECT_SOURCE_DIR}/source/utility.cpp"
	"${PROJECT_SOURCE_DIR}/source/utility.hpp"
	"${PROJECT_SOURCE_DIR}/source/strings.hpp"
	"${PROJECT_SOURCE_DIR}/source/codecs/hevc.hpp"
	"${PROJECT_SOURCE_DIR}/source/codecs/hevc.cpp"
	"${PROJECT_SOURCE_DIR}/source/codecs/h264.hpp"
	"${PROJECT_SOURCE_DIR}/source/codecs/h264.cpp"
	"${PROJECT_SOURCE_DIR}/source/codecs/prores.hpp"
	"${PROJECT_SOURCE_DIR}/source/codecs/prores.cpp"
	"${PROJECT_SOURCE_DIR}/source/ffmpeg/avframe-queue.cpp"
	"${PROJECT_SOURCE_DIR}/source/ffmpeg/avframe-queue.hpp"
	"${PROJECT_SOURCE_DIR}/source/ffmpeg/swscale.hpp"
	"${PROJECT_SOURCE_DIR}/source/ffmpeg/swscale.cpp"
	"${PROJECT_SOURCE_DIR}/source/ffmpeg/tools.hpp"
	"${PROJECT_SOURCE_DIR}/source/ffmpeg/tools.cpp"
	"${PROJECT_SOURCE_DIR}/source/hwapi/base.hpp"
	"${PROJECT_SOURCE_DIR}/source/hwapi/base.cpp"
	"${PROJECT_SOURCE_DIR}/source/ui/handler.hpp"
	"${PROJECT_SOURCE_DIR}/source/ui/handler.cpp"
	"${PROJECT_SOURCE_DIR}/source/ui/debug_handler.hpp"
	"${PROJECT_SOURCE_DIR}/source/ui/debug_handler.cpp"
	"${PROJECT_SOURCE_DIR}/source/ui/prores_aw_handler.hpp"
	"${PROJECT_SOURCE_DIR}/source/ui/prores_aw_handler.cpp"
	"${PROJECT_SOURCE_DIR}/source/ui/nvenc_shared.hpp"
	"${PROJECT_SOURCE_DIR}/source/ui/nvenc_shared.cpp"
	"${PROJECT_SOURCE_DIR}/source/ui/nvenc_h264_handler.hpp"
	"${PROJECT_SOURCE_DIR}/source/ui/nvenc_h264_handler.cpp"
	"${PROJECT_SOURCE_DIR}/source/ui/nvenc_hevc_handler.hpp"
	"${PROJECT_SOURCE_DIR}/source/ui/nvenc_hevc_handler.cpp"
)
if(WIN32)
	list(APPEND PROJECT_PRIVATE 
		"${PROJECT_SOURCE_DIR}/source/hwapi/d3d11.hpp"
		"${PROJECT_SOURCE_DIR}/source/hwapi/d3d11.cpp"
	)
endif()

# Source Grouping
source_group(TREE "${PROJECT_SOURCE_DIR}" PREFIX "Data Files" FILES ${PROJECT_DATA})
source_group(TREE "${PROJECT_BINARY_DIR}/source" PREFIX "Generated Files" FILES ${PROJECT_GENERATED})
source_group(TREE "${PROJECT_SOURCE_DIR}/cmake" PREFIX "Template Files" FILES ${PROJECT_TEMPLATES})

# Filter Sources
set(_TMP_SOURCE ${PROJECT_PRIVATE})
list(FILTER _TMP_SOURCE INCLUDE REGEX "\.(c|cpp)$")
source_group(TREE "${PROJECT_SOURCE_DIR}/source" PREFIX "Source Files" FILES ${_TMP_SOURCE})

# Filter Headers
set(_TMP_HEADER ${PROJECT_PRIVATE})
list(FILTER _TMP_HEADER INCLUDE REGEX "\.(h|hpp)$")
source_group(TREE "${PROJECT_SOURCE_DIR}/source" PREFIX "Header Files" FILES ${_TMP_HEADER})

################################################################################
# Target
################################################################################

add_library(${PROJECT_NAME} MODULE
	${PROJECT_GENERATED}
	${PROJECT_PRIVATE}
	${PROJECT_DATA}
	${PROJECT_TEMPLATES}
)

# Include Directories
target_include_directories(${PROJECT_NAME}
	PUBLIC
	PRIVATE
		"${PROJECT_BINARY_DIR}/source"
		"${PROJECT_SOURCE_DIR}/source"
		${FFMPEG_INCLUDE_DIRS}
)

# OBS Studio
if(${PropertyPrefix}OBS_NATIVE)
	target_link_libraries(${PROJECT_NAME}
		libobs
	)
elseif(${PropertyPrefix}OBS_REFERENCE)
	target_include_directories(${PROJECT_NAME}
		PRIVATE
			"${OBS_STUDIO_DIR}/libobs"
	)
	target_link_libraries(${PROJECT_NAME}
		"${LIBOBS_LIB}"
	)
elseif(${PropertyPrefix}OBS_PACKAGE)
	target_include_directories(${PROJECT_NAME}
		PRIVATE
			"${OBS_STUDIO_DIR}/include"
	)
	target_link_libraries(${PROJECT_NAME}
		libobs
	)
elseif(${PropertyPrefix}OBS_DOWNLOAD)
	target_link_libraries(${PROJECT_NAME}
		libobs
	)
endif()

# Link Libraries
target_link_libraries(${PROJECT_NAME}
	${PROJECT_LIBRARIES}
	${FFMPEG_LIBRARIES}
)

# Definitions
if (WIN32)
	target_compile_definitions(${PROJECT_NAME}
		PRIVATE
			_CRT_SECURE_NO_WARNINGS
			# windows.h
			WIN32_LEAN_AND_MEAN
			NOGPICAPMASKS
			NOVIRTUALKEYCODES
			#NOWINMESSAGES
			NOWINSTYLES
			NOSYSMETRICS
			NOMENUS
			NOICONS
			NOKEYSTATES
			NOSYSCOMMANDS
			NORASTEROPS
			NOSHOWWINDOW
			NOATOM
			NOCLIPBOARD
			NOCOLOR
			NOCTLMGR
			NODRAWTEXT
			#NOGDI
			NOKERNEL
			#NOUSER
			#NONLS
			NOMB
			NOMEMMGR
			NOMETAFILE
			NOMINMAX
			#NOMSG
			NOOPENFILE
			NOSCROLL
			NOSERVICE
			NOSOUND
			#NOTEXTMETRIC
			NOWH
			NOWINOFFSETS
			NOCOMM
			NOKANJI
			NOHELP
			NOPROFILER
			NODEFERWINDOWPOS
			NOMCX
			NOIME
			NOMDI
			NOINOUT
	)
endif()

# C++ Standard and Extensions
set_target_properties(
	${PROJECT_NAME}
	PROPERTIES
		CXX_STANDARD ${_CXX_STANDARD}
		CXX_EXTENSIONS ${_CXX_EXTENSIONS}
)

# File Version
if(WIN32)
	set_target_properties(
		${PROJECT_NAME}
		PROPERTIES
		VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}.${PROJECT_VERSION_TWEAK}
		SOVERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}.${PROJECT_VERSION_TWEAK}
	)
else()
	set_target_properties(
		${PROJECT_NAME}
		PROPERTIES
		VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}.${PROJECT_VERSION_TWEAK}
		SOVERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}.${PROJECT_VERSION_TWEAK}
	)
endif()

################################################################################
# Installation
################################################################################

if(${PropertyPrefix}OBS_NATIVE)
	install_obs_plugin_with_data(${PROJECT_NAME} data)
else()
	install(
		TARGETS ${PROJECT_NAME}
		RUNTIME DESTINATION "./obs-plugins/${BITS}bit/" COMPONENT Runtime
		LIBRARY DESTINATION "./obs-plugins/${BITS}bit/" COMPONENT Runtime
	)
	if(MSVC)
		install(
			FILES $<TARGET_PDB_FILE:${PROJECT_NAME}>
			DESTINATION "./obs-plugins/${BITS}bit/"
			OPTIONAL
		)
	endif()
	
	install(
		DIRECTORY "${PROJECT_SOURCE_DIR}/data/"
		DESTINATION "./data/obs-plugins/${PROJECT_NAME}/"
	)
	
	if("${CMAKE_PACKAGE_SUFFIX_OVERRIDE}" STREQUAL "")
		set(PackageFullName "${CMAKE_PACKAGE_PREFIX}/${CMAKE_PACKAGE_NAME}-${PROJECT_VERSION}")
	else()
		set(PackageFullName "${CMAKE_PACKAGE_PREFIX}/${CMAKE_PACKAGE_NAME}-${CMAKE_PACKAGE_SUFFIX_OVERRIDE}")
	endif()
	
	add_custom_target(
		PACKAGE_7Z
		${CMAKE_COMMAND} -E tar cfv "${PackageFullName}.7z" --format=7zip --
			"${CMAKE_INSTALL_PREFIX}/obs-plugins"
			"${CMAKE_INSTALL_PREFIX}/data"
		WORKING_DIRECTORY "${CMAKE_INSTALL_PREFIX}"
	)
	add_custom_target(
		PACKAGE_ZIP
		${CMAKE_COMMAND} -E tar cfv "${PackageFullName}.zip" --format=zip --
			"${CMAKE_INSTALL_PREFIX}/obs-plugins"
			"${CMAKE_INSTALL_PREFIX}/data"
		WORKING_DIRECTORY "${CMAKE_INSTALL_PREFIX}"
	)
endif()
