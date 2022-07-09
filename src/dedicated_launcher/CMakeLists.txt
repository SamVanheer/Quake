add_executable(quakeds)

target_include_directories(quakeds PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/..)

target_link_libraries(quakeds PRIVATE
	common
	stdlib
	Threads::Threads)

target_sources(quakeds PRIVATE
	Main.cpp)

if (MSVC)
	target_sources(quakeds PRIVATE
		../resource.h
		../winquake.rc)
endif()

#Create filters
get_target_property(TARGET_SOURCES quakeds SOURCES)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/.. FILES ${TARGET_SOURCES})

install(TARGETS quakeds RUNTIME DESTINATION .)

if (MSVC)
	install(FILES $<TARGET_PDB_FILE:quakeds> DESTINATION . OPTIONAL)
endif()
