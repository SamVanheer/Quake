add_executable(winquake WIN32)

target_include_directories(winquake PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/..)

target_link_libraries(winquake PRIVATE
	common
	stdlib
	SDL2::SDL2)

target_sources(winquake PRIVATE
	Main.cpp)

if (MSVC)
	target_sources(winquake PRIVATE
		resource.h
		winquake.rc)
endif()

#Create filters
get_target_property(TARGET_SOURCES winquake SOURCES)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${TARGET_SOURCES})

install(TARGETS winquake RUNTIME DESTINATION .)

if (MSVC)
	install(FILES $<TARGET_PDB_FILE:winquake> DESTINATION . OPTIONAL)
endif()
