add_library(stdlib STATIC)

target_include_directories(stdlib PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(stdlib PUBLIC
	common
	${CMAKE_DL_LIBS})

target_sources(stdlib PRIVATE
	IClientLauncher.h
	IDedicatedConsole.h
	IDedicatedLauncher.h)

# These need to be added to all users of this library.
target_sources(stdlib PUBLIC
	interface.cpp
	interface.h)

get_target_property(TARGET_SOURCES stdlib SOURCES)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/.. FILES ${TARGET_SOURCES})
