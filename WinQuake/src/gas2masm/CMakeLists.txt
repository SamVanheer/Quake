#
# gas2masm: GNU Assembly to Microsoft Macro Assembler converter
#

include(InputFilesList)

add_sources(
	gas2masm.cpp
)

preprocess_sources()

add_executable(gas2masm ${PREP_SRCS})

#Create filters
create_source_groups(${CMAKE_CURRENT_SOURCE_DIR})

clear_sources()

target_compile_definitions(gas2masm PRIVATE
	$<$<CXX_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>
	$<$<CXX_COMPILER_ID:MSVC>:$<$<CONFIG:Debug>:_DEBUG>>
	$<$<CXX_COMPILER_ID:MSVC>:_CONSOLE>)

if(MSVC)
	set_target_properties(gas2masm PROPERTIES
		COMPILE_FLAGS "/W3")
endif()
