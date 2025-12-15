
set(FMT_INSTALL ON)
set(EIGEN_BUILD_CMAKE_PACKAGE ON)
set(JSON_Install ON)
set(INCLUDE_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/include")

set(PNG_BUILD_ZLIB OFF)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-conversion -Wno-implicit-function-declaration")


set(ZLIB_INCLUDE_DIR  "${CMAKE_SOURCE_DIR}/ExternalLibs/zlib;${CMAKE_BINARY_DIR}/ExternalLibs/zlib" CACHE PATH "" FORCE)
set(ZLIB_LIBRARY      "${CMAKE_BINARY_DIR}/ExternalLibs/zlib/libz.a" CACHE FILEPATH "" FORCE)
set(ZLIB_LIBRARY_DEBUG   "${ZLIB_LIBRARY}" CACHE FILEPATH "" FORCE)
set(ZLIB_LIBRARY_RELEASE "${ZLIB_LIBRARY}" CACHE FILEPATH "" FORCE)

# Tell libpng to use your zlib and keep things static.
set(PNG_SHARED OFF CACHE BOOL "" FORCE)
set(PNG_STATIC ON  CACHE BOOL "" FORCE)
# If libpng’s tests aren’t needed on bare metal:
set(PNG_TOOLS  OFF CACHE BOOL "" FORCE)
set(PNG_TESTS  OFF CACHE BOOL "" FORCE)

# Now add an empty ZLIB::ZLIB interface library to stop find_package(ZLIB) from
# creating one. libpng will still add the target to its list of link libraries,
# but since it's an empty target, it will have no effect.
add_library(ZLIB::ZLIB INTERFACE IMPORTED)

option(PNG_SHARED "Build shared lib" OFF)
option(PNG_STATIC "Build static lib" ON)
option(PNG_TESTS "Build libpng tests" OFF)
#set(SKIP_INSTALL_ALL TRUE)

add_subdirectory(pugixml)
add_subdirectory(zlib)
add_subdirectory(libpng)
add_subdirectory(eigen)
add_subdirectory(fmt)
add_subdirectory(json)
add_subdirectory(googletest)

add_dependencies(png_static zlibstatic)

add_compile_definitions(FMT_OPTIMIZE_SIZE=2)
