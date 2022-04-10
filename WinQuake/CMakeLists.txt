#
# WinQuake
#

cmake_minimum_required(VERSION 3.21)

# Link statically with the runtime
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

project(WinQuake LANGUAGES C CXX)

add_subdirectory(src/winquake)

# Set Visual Studio starting project
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT winquake)
