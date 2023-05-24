add_library(libshvchainpack-c STATIC
	cchainpack.c
	ccpcp.c
	ccpcp_convert.c
	ccpon.c
	pack_double.c
	)
target_include_directories(libshvchainpack-c PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

function(add_shv_test test_name)
	add_executable(test_${test_name}
		tests/test_${test_name}.c
		)
	target_link_libraries(test_${test_name} libshvchainpack-c)
	add_test(NAME test_${test_name} COMMAND $<TARGET_FILE:test_${test_name}>)
endfunction(add_shv_test)

if(BUILD_TESTING)
	add_shv_test(ccpcp)
endif()

install(TARGETS libshvchainpack-c)
