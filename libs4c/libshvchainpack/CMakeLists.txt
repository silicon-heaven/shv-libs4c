add_library(libshvchainpack-c SHARED
    cchainpack.c
    ccpcp.c
    ccpcp_convert.c
    ccpon.c
    pack_double.c
    )
target_include_directories(libshvchainpack-c PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

install(TARGETS libshvchainpack-c)
