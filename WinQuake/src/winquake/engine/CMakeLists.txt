target_sources(winquake
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
		winquake.h
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
		
		client/sound/cdaudio.h
		client/sound/ISoundSystem.h
		client/sound/snd_dma.cpp
		client/sound/snd_mem.cpp
		client/sound/snd_mix.cpp
		client/sound/sound.h
		client/sound/SoundSystem.cpp
		client/sound/SoundSystem.h
		
		client/ui/menu.cpp
		client/ui/menu.h
		client/ui/sbar.cpp
		client/ui/sbar.h
		
		net/net.h
		net/net_dgrm.cpp
		net/net_dgrm.h
		net/net_loop.cpp
		net/net_loop.h
		net/net_main.cpp
		net/net_vcr.cpp
		net/net_vcr.h
		
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

if (MSVC)
	target_sources(winquake
		PRIVATE
			net/net_win.cpp
			net/net_wins.cpp
			net/net_wins.h
			plat/windows/cd_win.cpp
			server/conproc.cpp
			server/conproc.h)
endif()

if (UNIX)
	target_sources(winquake
		PRIVATE
			net/net_bsd.cpp
			net/net_udp.cpp
			net/net_udp.h
			plat/linux/cd_linux.cpp)
endif()

if(RENDERER_IS_SOFTWARE)
	add_subdirectory(client/renderer/software)
elseif(RENDERER_IS_OPENGL)
	add_subdirectory(client/renderer/opengl)
endif()
