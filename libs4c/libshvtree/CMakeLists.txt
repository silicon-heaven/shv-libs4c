cmake_minimum_required(VERSION 3.16)

set(SRCS shv_com.c shv_file_com.c shv_methods.c shv_tree.c shv_com_common.c)

if(DEFINED CONFIG_SHV_LIBS4C_PLATFORM)
    if(${CONFIG_SHV_LIBS4C_PLATFORM} EQUAL "linux")
        find_package(ZLIB REQUIRED)
        set(SRCS ${SRCS} shv_clayer_posix.c shv_clayer_linux.c shv_clayer_posix.h)
    elseif(${CONFIG_SHV_LIBS4C_PLATFORM} EQUAL "nuttx")
        set(SRCS ${SRCS} shv_clayer_posix.c shv_clayer_nuttx.c shv_clayer_posix.h)
    endif()
else()
    message(FATAL_ERROR "Define CONFIG_SHV_LIBS4C_PLATFORM: either \"linux\" or \"nuttx\".")
endif()

add_library(shvtree STATIC ${SRCS})
target_link_libraries(shvtree PUBLIC ulut)
target_link_libraries(shvtree PUBLIC shvchainpack)
# linux must be linked with zlib
if(${CONFIG_SHV_LIBS4C_PLATFORM} EQUAL "linux")
    target_link_libraries(shvtree PUBLIC ZLIB)
endif()

target_include_directories(shvtree PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
