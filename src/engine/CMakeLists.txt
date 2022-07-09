find_package(Threads REQUIRED)
find_package(OpenGL REQUIRED)
find_package(OpenAL CONFIG REQUIRED)
find_package(Ogg CONFIG REQUIRED)
find_package(Vorbis CONFIG REQUIRED)

option(BUILD_STATIC_LIB "" ON)
option(BUILD_SHARED_LIB "" OFF)
option(BUILD_EXAMPLES "" OFF)
option(BUILD_TESTS "" OFF)
option(BUILD_TOOLS "" OFF)
option(ENABLE_ICE "" OFF)
option(Protobuf_USE_STATIC_LIBS "" ON)
option(MSVC_CRT_STATIC "" ON)
add_subdirectory(../external/GameNetworkingSockets
	GameNetworkingSockets-binary-dir
	EXCLUDE_FROM_ALL)

set_target_properties(GameNetworkingSockets_s PROPERTIES FOLDER "Dependencies")

add_library(engine SHARED)

set_target_properties(engine PROPERTIES PREFIX "")

target_precompile_headers(engine PRIVATE quakedef.h)

target_include_directories(engine PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR})

if(RENDERER_IS_OPENGL)
	target_compile_definitions(engine PRIVATE
		GLQUAKE)
endif()

target_link_libraries(engine PRIVATE
	common
	stdlib
	$<$<CXX_COMPILER_ID:MSVC>:Ws2_32>
	Threads::Threads
	SDL2::SDL2
	OpenGL::GLU
	OpenGL::GL
	OpenAL::OpenAL
	Ogg::ogg
	Vorbis::vorbis
	Vorbis::vorbisfile
	GameNetworkingSockets_s)

target_sources(engine
	PRIVATE
		anorm_dots.h
		anorms.h
		bspfile.h
		cmd.cpp
		cmd.h
		common.cpp
		common.h
		console.cpp
		console.h
		crc.cpp
		crc.h
		cvar.cpp
		cvar.h
		host.cpp
		host_cmd.cpp
		mathlib.cpp
		mathlib.h
		modelgen.h
		quakedef.h
		protocol.h
		quakedef.h
		spritegn.h
		sys.cpp
		sys.h
		wad.cpp
		wad.h
		zone.cpp
		zone.h
		
		client/chase.cpp
		client/cl_demo.cpp
		client/cl_input.cpp
		client/cl_main.cpp
		client/cl_parse.cpp
		client/cl_tent.cpp
		client/client.h
		client/input.cpp
		client/input.h
		client/keys.cpp
		client/keys.h
		client/vid.cpp
		client/vid.h
		client/view.cpp
		client/view.h
		
		client/renderer/draw.h
		client/renderer/r_part.cpp
		client/renderer/render.h
		client/renderer/screen.h
		
		client/sound/CDAudio.cpp
		client/sound/CDAudio.h
		client/sound/ICDAudio.h
		client/sound/ISoundSystem.h
		client/sound/OggSoundLoader.cpp
		client/sound/OggSoundLoader.h
		client/sound/snd_mem.cpp
		client/sound/sound.cpp
		client/sound/sound.h
		client/sound/sound_internal.h
		client/sound/SoundSystem.cpp
		client/sound/SoundSystem.h
		client/sound/WaveSoundLoader.cpp
		client/sound/WaveSoundLoader.h
		
		client/ui/menu.cpp
		client/ui/menu.h
		client/ui/sbar.cpp
		client/ui/sbar.h
		
		net/CGNSNetSystem.cpp
		net/CGNSNetSystem.h
		net/net.h
		net/net_main.cpp
		
		server/server.h
		server/sv_main.cpp
		server/sv_move.cpp
		server/sv_phys.cpp
		server/sv_user.cpp
		server/world.cpp
		server/world.h
		
		server/progs/pr_cmds.cpp
		server/progs/pr_edict.cpp
		server/progs/progdefs.h
		server/progs/progs.h)

if(RENDERER_IS_SOFTWARE)
	add_subdirectory(client/renderer/software)
elseif(RENDERER_IS_OPENGL)
	add_subdirectory(client/renderer/opengl)
endif()

add_subdirectory(game)

#Create filters
get_target_property(TARGET_SOURCES engine SOURCES)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/.. FILES ${TARGET_SOURCES})

install(FILES $<TARGET_FILE:SDL2::SDL2> DESTINATION .)
install(FILES $<TARGET_FILE:OpenAL::OpenAL> DESTINATION .)
	
install(TARGETS engine RUNTIME DESTINATION .)

if (MSVC)
	install(FILES $<TARGET_PDB_FILE:engine> DESTINATION . OPTIONAL)
endif()
