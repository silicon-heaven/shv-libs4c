cmake_minimum_required(VERSION 3.16)

add_library(shvchainpack STATIC cchainpack.c ccpcp.c ccpcp_convert.c ccpon.c)
target_include_directories(shvchainpack PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

function(add_shv_test test_name)
	add_executable(test_${test_name} tests/test_${test_name}.c)
	target_link_libraries(test_${test_name} shvchainpack)
	add_test(NAME test_${test_name} COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:test_${test_name}>)
endfunction(add_shv_test)

if(BUILD_TESTING)
	add_shv_test(ccpcp)
endif()
