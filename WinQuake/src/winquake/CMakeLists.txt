#
# WinQuake game
#

find_package(OpenGL REQUIRED)

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
		cdaudio.h
		chase.cpp
		client.h
		cmd.cpp
		cmd.h
		common.cpp
		common.h
		conproc.cpp
		conproc.h
		console.cpp
		console.h
		crc.cpp
		crc.h
		cvar.cpp
		cvar.h
		dosisms.h
		draw.h
		host.cpp
		host_cmd.cpp
		in_win.cpp
		input.h
		keys.cpp
		keys.h
		math.s
		mathlib.cpp
		mathlib.h
		menu.cpp
		menu.h
		modelgen.h
		pr_cmds.cpp
		pr_comp.h
		pr_edict.cpp
		pr_exec.cpp
		progdefs.h
		progs.h
		protocol.h
		quakeasm.h
		quakedef.h
		r_part.cpp
		render.h
		sbar.cpp
		sbar.h
		screen.h
		server.h
		snd_dma.cpp
		snd_mem.cpp
		snd_mix.cpp
		snd_mixa.s
		sound.h
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
		worlda.s
		zone.cpp
		zone.h
		renderer/d_ifacea.h
		progdefs.q1
		progdefs.q2
		$<$<CXX_COMPILER_ID:MSVC>:
			cd_win.cpp
			snd_win.cpp
			sys_win.cpp
			sys_wina.s
			winquake.rc
		>
		$<$<PLATFORM_ID:UNIX>:
			cd_linux.cpp
			snd_linux.cpp
			sys_dosa.s
			sys_linux.cpp
		>)

add_subdirectory(client)
add_subdirectory(net)
add_subdirectory(server)

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
	#$<$<CXX_COMPILER_ID:MSVC>:${CMAKE_SOURCE_DIR}/external/dxsdk/sdk/inc>
)

# TODO: replace with the generator expression above when assembly files have been converted - Solokiller
if(MSVC)
	target_include_directories(winquake PRIVATE
		${CMAKE_SOURCE_DIR}/external/dxsdk/sdk/inc)
endif()

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
	$<$<CXX_COMPILER_ID:MSVC>:${CMAKE_SOURCE_DIR}/external/dxsdk/sdk/lib/dxguid.lib>)

if(RENDERER_IS_SOFTWARE)
	target_link_libraries(winquake PRIVATE
		${CMAKE_SOURCE_DIR}/external/scitech/lib/win32/vc/mgllt.lib)
endif()

if(RENDERER_IS_OPENGL)
	target_link_libraries(winquake PRIVATE
		OpenGL::GLU
		OpenGL::GL)
endif()

if(MSVC)
	#Software mode tries to link with this, but it doesn't appear to exist for new versions of MSVC
	if(RENDERER_IS_SOFTWARE)
		set(IGNORE_LIBC "/NODEFAULTLIB:LIBC.lib")
	endif()
	
	target_compile_options(winquake
		PRIVATE
			# Disable warning 4805: '==': unsafe mix of type 'qboolean' and type 'bool' in operation
			# TODO: once all code has upgraded away from qboolean, remove this
			# Disable warning 4244: '=': conversion from 'type1' to 'type2', possible loss of data
			# Disable warning 4100: 'parameter': unreferenced formal parameter
			# Disable warning 4459: declaration of 'local variable' hides global declaration
			# Disable warning 4611: interaction between '_setjmp' and C++ object destruction is non-portable TODO remove setjmp and use proper error handling
			/W4 /wd4805 /wd4244 /wd4100 /wd4459 /wd4611)
	
	target_link_options(winquake
		PRIVATE
			# /SAFESEH:NO is needed because of assembly files. Can be removed once those are gone
			/SAFESEH:NO ${IGNORE_LIBC})
endif()

# TODO: for now we need to know which directories to look in, needs to be removed once assembly files are converted - Solokiller
get_target_property(include_dirs winquake INCLUDE_DIRECTORIES)

foreach(dir IN LISTS include_dirs)
	set(ASM_INCLUDE_DIRS ${ASM_INCLUDE_DIRS} /I ${dir})
endforeach()

# Until we get rid of assembly code we'll need to add these custom builds
function(configure_asm_file filename)
	set(OUT_FILENAME ${CMAKE_CURRENT_BINARY_DIR}/${filename})
	
	if(MSVC)
		# Some commands can't handle forward slashes, so make sure they're all replaced
		string(REGEX REPLACE "/" "\\\\" OUT_FILENAME ${OUT_FILENAME}) 
		
		add_custom_command(
			OUTPUT ${filename}.obj
			MAIN_DEPENDENCY ${filename}.s
			COMMAND cl ${ASM_INCLUDE_DIRS} /EP > ${OUT_FILENAME}.spp ${CMAKE_CURRENT_SOURCE_DIR}\\${filename}.s
			COMMAND $<TARGET_FILE:gas2masm> < ${OUT_FILENAME}.spp > ${OUT_FILENAME}.asm
			COMMAND ml /c /Cp /coff /Fo${OUT_FILENAME}.obj /Zm /Zi ${OUT_FILENAME}.asm
			COMMAND del ${OUT_FILENAME}.spp)
		
		target_sources(winquake PRIVATE ${filename}.obj)
	endif()
endfunction()

configure_asm_file(math)
configure_asm_file(snd_mixa)
configure_asm_file(sys_wina)
configure_asm_file(worlda)

if(UNIX)
	configure_asm_file(sys_dosa)
endif()

# Software renderer specific assembly files
if(RENDERER_IS_SOFTWARE)
	configure_asm_file(renderer/software/d_draw)
	configure_asm_file(renderer/software/d_draw16)
	configure_asm_file(renderer/software/d_parta)
	configure_asm_file(renderer/software/d_polysa)
	configure_asm_file(renderer/software/d_scana)
	configure_asm_file(renderer/software/d_spr8)
	configure_asm_file(renderer/software/d_varsa)
	configure_asm_file(renderer/software/r_aclipa)
	configure_asm_file(renderer/software/r_aliasa)
	configure_asm_file(renderer/software/r_drawa)
	configure_asm_file(renderer/software/r_edgea)
	configure_asm_file(renderer/software/r_varsa)
	configure_asm_file(renderer/software/surf8)
	configure_asm_file(renderer/software/surf16)
endif()

set_target_properties(winquake PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY ${CMAKE_INSTALL_PREFIX})

foreach(config IN LISTS CMAKE_CONFIGURATION_TYPES)
	# The property names are all caps
	string(TOUPPER ${config} config)
	
	set_target_properties(winquake PROPERTIES
		RUNTIME_OUTPUT_DIRECTORY_${config} ${CMAKE_INSTALL_PREFIX})
endforeach()
