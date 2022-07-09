add_library(common INTERFACE)

target_include_directories(common INTERFACE
	${CMAKE_CURRENT_SOURCE_DIR}/..)

target_compile_definitions(common INTERFACE
	$<$<CXX_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>
	$<$<CXX_COMPILER_ID:MSVC>:$<$<CONFIG:Debug>:_DEBUG>>)

target_compile_options(common
	INTERFACE
		$<$<CXX_COMPILER_ID:MSVC>:/MP>)

if(MSVC)	
	target_compile_options(common
		INTERFACE
			# Disable warning 4244: '=': conversion from 'type1' to 'type2', possible loss of data
			# Disable warning 4100: 'parameter': unreferenced formal parameter
			# Disable warning 4459: declaration of 'local variable' hides global declaration
			# Disable warning 4611: interaction between '_setjmp' and C++ object destruction is non-portable TODO remove setjmp and use proper error handling
			/W4 /wd4244 /wd4100 /wd4459 /wd4611)
endif()

target_sources(common INTERFACE Platform.h)
