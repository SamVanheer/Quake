target_sources(winquake
	PRIVATE
		net.h
		net_dgrm.cpp
		net_dgrm.h
		net_loop.cpp
		net_loop.h
		net_main.cpp
		net_vcr.cpp
		net_vcr.h)

if (MSVC)
target_sources(winquake
	PRIVATE
		net_win.cpp
		net_wins.cpp
		net_wins.h)
endif()

if (UNIX)
target_sources(winquake
	PRIVATE
		net_bsd.cpp
		net_udp.cpp
		net_udp.h)
endif()
