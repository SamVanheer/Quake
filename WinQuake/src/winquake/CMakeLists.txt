#
# WinQuake game
#

find_package(OpenGL REQUIRED)

add_subdirectory(../../external/SDL2
	SDL2-binary-dir
	EXCLUDE_FROM_ALL)

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

target_sources(winquake
	PRIVATE
		anorm_dots.h
		anorms.h
		bspfile.h
		chase.cpp
		client.h
		cmd.cpp
		cmd.h
		common.cpp
		common.h
		$<$<CXX_COMPILER_ID:MSVC>:
			conproc.cpp
			conproc.h
		>
		console.cpp
		console.h
		crc.cpp
		crc.h
		cvar.cpp
		cvar.h
		draw.h
		host.cpp
		host_cmd.cpp
		input.h
		keys.cpp
		keys.h
		mathlib.cpp
		mathlib.h
		modelgen.h
		quakedef.h
		pr_cmds.cpp
		pr_comp.h
		pr_edict.cpp
		pr_exec.cpp
		progdefs.h
		progs.h
		protocol.h
		quakedef.h
		server.h
		spritegn.h
		sys.h
		vid.h
		view.cpp
		view.h
		wad.cpp
		wad.h
		winquake.h
		world.cpp
		world.h
		zone.cpp
		zone.h
		progdefs.q1
		progdefs.q2
		renderer/r_part.cpp
		renderer/render.h
		renderer/screen.h
		$<$<CXX_COMPILER_ID:MSVC>:winquake.rc>)

add_subdirectory(client)
add_subdirectory(net)
add_subdirectory(server)

if (MSVC)
	add_subdirectory(plat/windows)
else()
	add_subdirectory(plat/linux)
endif()

if(RENDERER_IS_SOFTWARE)
	add_subdirectory(renderer/software)
elseif(RENDERER_IS_OPENGL)
	add_subdirectory(renderer/opengl)
endif()

#Create filters
get_target_property(TARGET_SOURCES winquake SOURCES)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${TARGET_SOURCES})

target_include_directories(winquake PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}
	$<$<CXX_COMPILER_ID:MSVC>:${CMAKE_SOURCE_DIR}/external/dxsdk/sdk/inc>
)

if(RENDERER_IS_SOFTWARE)
	target_include_directories(winquake PRIVATE
		${CMAKE_SOURCE_DIR}/external/scitech/include)
endif()

target_compile_definitions(winquake PRIVATE
	$<$<CXX_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>
	$<$<CXX_COMPILER_ID:MSVC>:$<$<CONFIG:Debug>:_DEBUG>>
	$<$<CXX_COMPILER_ID:MSVC>:CINTERFACE> #DirectX interfaces need this (for now)
)

if(RENDERER_IS_OPENGL)
	target_compile_definitions(winquake PRIVATE
		GLQUAKE)
endif()

target_link_libraries(winquake PRIVATE
	$<$<CXX_COMPILER_ID:MSVC>:winmm>
	$<$<CXX_COMPILER_ID:MSVC>:wsock32>
	$<$<CXX_COMPILER_ID:MSVC>:comctl32>
	$<$<CXX_COMPILER_ID:MSVC>:${CMAKE_SOURCE_DIR}/external/dxsdk/sdk/lib/dxguid.lib>
	SDL2::SDL2)

if(RENDERER_IS_SOFTWARE)
	target_link_libraries(winquake PRIVATE
		${CMAKE_SOURCE_DIR}/external/scitech/lib/win32/vc/mgllt.lib)
endif()

if(RENDERER_IS_OPENGL)

	if (UNIX)
		find_package(svgalib REQUIRED)
	endif()
	
	target_link_libraries(winquake PRIVATE
		OpenGL::GLU
		OpenGL::GL
		$<$<PLATFORM_ID:Linux>:svgalib>)
endif()

if(MSVC)	
	target_compile_options(winquake
		PRIVATE
			# Disable warning 4805: '==': unsafe mix of type 'qboolean' and type 'bool' in operation
			# TODO: once all code has upgraded away from qboolean, remove this
			# Disable warning 4244: '=': conversion from 'type1' to 'type2', possible loss of data
			# Disable warning 4100: 'parameter': unreferenced formal parameter
			# Disable warning 4459: declaration of 'local variable' hides global declaration
			# Disable warning 4611: interaction between '_setjmp' and C++ object destruction is non-portable TODO remove setjmp and use proper error handling
			/W4 /wd4805 /wd4244 /wd4100 /wd4459 /wd4611)
	
	if (RENDERER_IS_SOFTWARE)
		target_link_options(winquake
			PRIVATE
				# /SAFESEH:NO is needed because of mgllt.
				# Software mode tries to link with LIBC, but it doesn't exist in new versions of MSVC
				/SAFESEH:NO /NODEFAULTLIB:LIBC.lib)
	endif()
endif()

if (UNIX)
	target_compile_options(winquake
		PRIVATE
			-Wno-write-strings)
endif()

install(TARGETS SDL2
	RUNTIME DESTINATION .)

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
		
