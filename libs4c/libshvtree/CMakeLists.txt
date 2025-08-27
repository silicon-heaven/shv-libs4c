cmake_minimum_required(VERSION 3.16)

# Evaluate the source files
set(SRCS shv_com.c shv_file_node.c shv_methods.c shv_tree.c shv_com_common.c shv_connection.c shv_dotdevice_node.c)
if(DEFINED CONFIG_SHV_LIBS4C_PLATFORM)
    if(${CONFIG_SHV_LIBS4C_PLATFORM} STREQUAL "linux")
        find_package(ZLIB REQUIRED)
        list(APPEND SRCS shv_clayer_posix.c)
    elseif(${CONFIG_SHV_LIBS4C_PLATFORM} STREQUAL "nuttx")
        list(APPEND SRCS shv_clayer_posix.c)
    endif()
else()
    message(FATAL_ERROR "Define CONFIG_SHV_LIBS4C_PLATFORM: either \"linux\" or \"nuttx\".")
endif()

add_library(shvtree STATIC ${SRCS})

if(${CONFIG_SHV_LIBS4C_PLATFORM} STREQUAL "linux")
    target_compile_definitions(shvtree PRIVATE CONFIG_SHV_LIBS4C_PLATFORM_LINUX)
    target_link_libraries(shvtree PUBLIC z) # link zlib on linux
elseif(${CONFIG_SHV_LIBS4C_PLATFORM} STREQUAL "nuttx")
    target_compile_definitions(shvtree PRIVATE CONFIG_SHV_LIBS4C_PLATFORM_NUTTX)
endif()

target_link_libraries(shvtree PUBLIC m) # we need pow
target_link_libraries(shvtree PUBLIC ulut)
target_link_libraries(shvtree PUBLIC shvchainpack)

target_include_directories(shvtree PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
