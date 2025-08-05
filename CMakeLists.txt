cmake_minimum_required(VERSION 3.16)

# The ulut library must be built beforehand as it does not feature CMake.
# Use ExternalProject to build it. Since the Ulut OMK system already performs the install,
# we just tell CMake where to look for includes and the compiled library.
# The problem is that we need to change the include format a bit.

# We expect the Ulut's include structure to have everything inside ulut/.
# But Ulut's OMK does not create this structure automatically.
# So we create a dummy ulut directory inside the include directory


set(ULUT_SRC_DIR     ${CMAKE_CURRENT_SOURCE_DIR}/submodule/ulut)
set(ULUT_LIB_DIR     ${ULUT_SRC_DIR}/_compiled/lib)
set(ULUT_INCLUDE_DIR ${ULUT_SRC_DIR}/_compiled/include)

# The directories must be created beforehand. Otherwise CMake complains.
file(MAKE_DIRECTORY ${ULUT_LIB_DIR})
file(MAKE_DIRECTORY ${ULUT_INCLUDE_DIR})

include(ExternalProject)
ExternalProject_Add(ulut_build
    BINARY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/submodule/ulut
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/submodule/ulut
    INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}
    # The structure is a bit different so we need to make this copy.
    INSTALL_COMMAND cmake -E copy_directory ${ULUT_INCLUDE_DIR} ${ULUT_INCLUDE_DIR}/ulut
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ${CMAKE_COMMAND} -E env
        CC=${CMAKE_C_COMPILER}
        CXX=${CMAKE_CXX_COMPILER}
        CFLAGS=${CMAKE_C_FLAGS}
        CXXFLAGS=${CMAKE_CXX_FLAGS}
        make
)

# Add ulut with custom paths
add_library(ulut STATIC IMPORTED)
# ulut_build is a dummy target, create a real library
add_dependencies(ulut ulut_build)
set_target_properties(ulut PROPERTIES
    IMPORTED_LOCATION ${ULUT_LIB_DIR}/libulut.a
    INTERFACE_INCLUDE_DIRECTORIES ${ULUT_INCLUDE_DIR}
)

add_subdirectory(libs4c)
# Create a libshv library that has everything needed
add_library(libshv INTERFACE)
target_link_libraries(libshv INTERFACE ulut shvtree shvchainpack)
