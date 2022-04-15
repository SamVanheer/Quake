#
# WinQuake game
#

find_package(OpenGL REQUIRED)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

add_subdirectory(../../external/SDL2
	SDL2-binary-dir
	EXCLUDE_FROM_ALL)

option(ALSOFT_UTILS  "" OFF)
add_subdirectory(../../external/OpenAL
	OpenAL-binary-dir
	EXCLUDE_FROM_ALL)

set_target_properties(
	SDL2
	build_version
	common
	OpenAL
	PROPERTIES
		FOLDER "Dependencies")

target_compile_options(common PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/MP>)
target_compile_options(OpenAL PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/MP>)

set(WINQUAKE_RENDERER "OpenGL" CACHE STRING "Renderer to use")
set_property(CACHE WINQUAKE_RENDERER PROPERTY STRINGS Software OpenGL)

if(${WINQUAKE_RENDERER} STREQUAL "Software")
	set(RENDERER_IS_SOFTWARE ON)
elseif(${WINQUAKE_RENDERER} STREQUAL "OpenGL")
	set(RENDERER_IS_OPENGL ON)
else()
	message(FATAL_ERROR "Unknown renderer ${WINQUAKE_RENDERER} selected")
endif()

add_executable(winquake WIN32)

add_subdirectory(engine)

if (MSVC)
	target_sources(winquake
		PRIVATE
			resource.h
			winquake.rc)
endif()

target_precompile_headers(winquake PRIVATE engine/quakedef.h)

#Create filters
get_target_property(TARGET_SOURCES winquake SOURCES)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${TARGET_SOURCES})

target_include_directories(winquake PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/engine
)

target_compile_definitions(winquake PRIVATE
	$<$<CXX_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>
	$<$<CXX_COMPILER_ID:MSVC>:$<$<CONFIG:Debug>:_DEBUG>>
)

if(RENDERER_IS_OPENGL)
	target_compile_definitions(winquake PRIVATE
		GLQUAKE)
endif()

target_link_libraries(winquake PRIVATE
	$<$<CXX_COMPILER_ID:MSVC>:winmm>
	$<$<CXX_COMPILER_ID:MSVC>:Ws2_32>
	SDL2::SDL2
	OpenGL::GLU
	OpenGL::GL
	OpenAL)

target_compile_options(winquake
	PRIVATE
		$<$<CXX_COMPILER_ID:MSVC>:/MP>)

if(MSVC)	
	target_compile_options(winquake
		PRIVATE
			# Disable warning 4244: '=': conversion from 'type1' to 'type2', possible loss of data
			# Disable warning 4100: 'parameter': unreferenced formal parameter
			# Disable warning 4459: declaration of 'local variable' hides global declaration
			# Disable warning 4611: interaction between '_setjmp' and C++ object destruction is non-portable TODO remove setjmp and use proper error handling
			/W4 /wd4244 /wd4100 /wd4459 /wd4611)
endif()

install(TARGETS SDL2
	RUNTIME DESTINATION .
	LIBRARY DESTINATION .)

install(TARGETS OpenAL
	RUNTIME DESTINATION .
	LIBRARY DESTINATION .)
	
install(TARGETS winquake
	RUNTIME DESTINATION .)

if (MSVC)
	install(FILES $<TARGET_PDB_FILE:winquake> DESTINATION . OPTIONAL)
endif()

# Set up debugger to launch the game instead of the copy in the build directory
set_target_properties(winquake
	PROPERTIES
		VS_DEBUGGER_COMMAND ${CMAKE_INSTALL_PREFIX}/winquake$<TARGET_FILE_SUFFIX:winquake>
		VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX}
		VS_DEBUGGER_COMMAND_ARGUMENTS "-window")
		
