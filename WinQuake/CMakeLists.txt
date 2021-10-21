#
# WinQuake
#

# Source additional modules from the "cmake" directory
list(APPEND CMAKE_MODULE_PATH
	"${CMAKE_CURRENT_LIST_DIR}/../cmake")

cmake_minimum_required(VERSION 3.21)

project(WinQuake LANGUAGES C CXX)

add_subdirectory(src/gas2masm)
add_subdirectory(src/winquake)
