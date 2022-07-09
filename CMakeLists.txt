#
# WinQuake
#

cmake_minimum_required(VERSION 3.21)

set(VCPKG_OVERLAY_TRIPLETS ${CMAKE_SOURCE_DIR}/vcpkg_config/triplets)

# Link statically with the runtime
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

project(WinQuake LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(THREADS_PREFER_PTHREAD_FLAG ON)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(WINQUAKE_RENDERER "OpenGL" CACHE STRING "Renderer to use")
set_property(CACHE WINQUAKE_RENDERER PROPERTY STRINGS Software OpenGL)

if(${WINQUAKE_RENDERER} STREQUAL "Software")
	set(RENDERER_IS_SOFTWARE ON)
elseif(${WINQUAKE_RENDERER} STREQUAL "OpenGL")
	set(RENDERER_IS_OPENGL ON)
else()
	message(FATAL_ERROR "Unknown renderer ${WINQUAKE_RENDERER} selected")
endif()

add_subdirectory(src)

# Set up debugger to launch the game instead of the copy in the build directory
set_target_properties(winquake
	PROPERTIES
		VS_DEBUGGER_COMMAND ${CMAKE_INSTALL_PREFIX}/winquake$<TARGET_FILE_SUFFIX:winquake>
		VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX}
		VS_DEBUGGER_COMMAND_ARGUMENTS "-window")

# Set Visual Studio starting project
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT winquake)
