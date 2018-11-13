# A Plugin that integrates the AMD AMF encoder into OBS Studio
# Copyright (C) 2016 - 2017 Michael Fabian Dirks
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA

# CMake Setup
cmake_minimum_required(VERSION 3.1.0)
include("cmake/util.cmake")

# Define Project
project(
	enc-ffmpeg
	VERSION 1.0.0.0
)
set(PROJECT_FULL_NAME "FFMPEG Encoder for OBS Studio")
set(PROJECT_DESCRIPTION "Plugin for OBS Studio to add FFMPEG options for Recording and Streaming")
set(PROJECT_AUTHORS "Michael Fabian 'Xaymar' Dirks <info@xaymar.com>")
set(PROJECT_COPYRIGHT_YEARS "2018")

################################################################################
# CMake / Compiler
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

# Configure Version Header
configure_file(
	"${PROJECT_SOURCE_DIR}/cmake/version.hpp.in"
	"${PROJECT_BINARY_DIR}/source/version.hpp"
)
configure_file(
	"${PROJECT_SOURCE_DIR}/cmake/module.cpp.in"
	"${PROJECT_BINARY_DIR}/source/module.cpp"
)

# Windows Specific Resource Definition
if(WIN32)
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

# All Warnings, Extra Warnings, Pedantic
if(MSVC)
	# Force to always compile with W4
	if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
		string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	else()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
	endif()
elseif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
	# Update if necessary
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-long-long -pedantic")
endif()

math(EXPR BITS "8*${CMAKE_SIZEOF_VOID_P}")

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
	set(${PropertyPrefix}OBS_DOWNLOAD_VERSION "22.0.2" CACHE STRING "OBS Studio Version to download")
endif()

if(NOT ${PropertyPrefix}OBS_NATIVE)
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
		set(${PropertyPrefix}OBS_DOWNLOAD_URL "https://github.com/Xaymar/obs-studio/releases/download/${OBS_DOWNLOAD_VERSION}/obs-studio-${ARCH}-vs2017.7z")
	endif()
endif()

# CMake Modules
if(${PropertyPrefix}OBS_DOWNLOAD)
	include("cmake/DownloadProject.cmake")
endif()
if(NOT ${PropertyPrefix}OBS_NATIVE)
	include("cmake/cppcheck.cmake")
endif()

# Load OBS Studio
if(${PropertyPrefix}OBS_NATIVE)
	Option(BUILD_FFMPEG_ENCODER "Build AMD Encoder module" ON)	
	if (NOT BUILD_FFMPEG_ENCODER)
		message(STATUS "Not building AMD Encoder")
		return()
	endif()
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
	message(CRITICAL "Impossible case reached, very system stability.")
	return()
endif()

# FFmpeg
find_package(FFmpeg REQUIRED COMPONENTS avutil avcodec swscale)

################################################################################
# Code
################################################################################
set(PROJECT_PRIVATE
	"${PROJECT_SOURCE_DIR}/source/encoder.cpp"
	"${PROJECT_SOURCE_DIR}/source/encoder.hpp"
	"${PROJECT_SOURCE_DIR}/source/plugin.cpp"
	"${PROJECT_SOURCE_DIR}/source/plugin.hpp"
	"${PROJECT_SOURCE_DIR}/source/utility.cpp"
	"${PROJECT_SOURCE_DIR}/source/utility.hpp"
	"${PROJECT_SOURCE_DIR}/source/encoders/prores_aw.hpp"
	"${PROJECT_SOURCE_DIR}/source/encoders/prores_aw.cpp"
	"${PROJECT_SOURCE_DIR}/source/encoders/prores_ks.hpp"
	"${PROJECT_SOURCE_DIR}/source/encoders/prores_ks.cpp"
	"${PROJECT_SOURCE_DIR}/source/ffmpeg/avframe-queue.cpp"
	"${PROJECT_SOURCE_DIR}/source/ffmpeg/avframe-queue.hpp"
	"${PROJECT_SOURCE_DIR}/source/ffmpeg/swscale.hpp"
	"${PROJECT_SOURCE_DIR}/source/ffmpeg/swscale.cpp"
	"${PROJECT_SOURCE_DIR}/source/ffmpeg/tools.hpp"
	"${PROJECT_SOURCE_DIR}/source/ffmpeg/tools.cpp"
)
SET(PROJECT_GENERATED
	"${PROJECT_BINARY_DIR}/source/module.cpp"
	"${PROJECT_BINARY_DIR}/source/version.hpp"
)
set(PROJECT_DATA
	"${PROJECT_SOURCE_DIR}/data/locale/en-US.ini"
	"${PROJECT_SOURCE_DIR}/LICENSE"
)
set(PROJECT_LIBRARIES
)

# Source Grouping
source_group(TREE "${PROJECT_SOURCE_DIR}" PREFIX "Data Files" FILES ${PROJECT_DATA})
source_group(TREE "${PROJECT_SOURCE_DIR}/source" PREFIX "Private Files" FILES ${PROJECT_PRIVATE})
source_group(TREE "${PROJECT_BINARY_DIR}/source" PREFIX "Generated Files" FILES ${PROJECT_GENERATED})

################################################################################
# Target
################################################################################

add_library(${PROJECT_NAME} MODULE
	${PROJECT_GENERATED}
	${PROJECT_PRIVATE}
	${PROJECT_DATA}
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
	"${PROJECT_LIBRARIES}"
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

# CPPCheck
if(NOT ${PropertyPrefix}OBS_NATIVE)
	set(excludes )
	list(APPEND excludes "${PROJECT_SOURCE_DIR}/AMF")
	if(${PropertyPrefix}OBS_REFERENCE)
		list(APPEND excludes "${OBS_STUDIO_DIR}/libobs")
	elseif(${PropertyPrefix}OBS_PACKAGE)
		list(APPEND excludes "${OBS_STUDIO_DIR}/libobs")
	elseif(${PropertyPrefix}OBS_DOWNLOAD)
		list(APPEND excludes "${libobs_SOURCE_DIR}")
	endif()

	cppcheck(
		EXCLUDE ${excludes}
	)
	cppcheck_add_project(${PROJECT_NAME})
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
		DIRECTORY "${PROJECT_SOURCE_DIR}/resources/"
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
