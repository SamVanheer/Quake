#
# gas2masm: GNU Assembly to Microsoft Macro Assembler converter
#

add_executable(gas2masm gas2masm.cpp)

#Create filters
get_target_property(TARGET_SOURCES gas2masm SOURCES)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${TARGET_SOURCES})

target_compile_definitions(gas2masm PRIVATE
	$<$<CXX_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>
	$<$<CXX_COMPILER_ID:MSVC>:$<$<CONFIG:Debug>:_DEBUG>>
	$<$<CXX_COMPILER_ID:MSVC>:_CONSOLE>)

if(MSVC)
	set_target_properties(gas2masm PROPERTIES
		COMPILE_FLAGS "/W3")
endif()
