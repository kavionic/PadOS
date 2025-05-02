
set(PNG_BUILD_ZLIB OFF)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-conversion -Wno-implicit-function-declaration")

add_subdirectory(pugixml EXCLUDE_FROM_ALL)
add_subdirectory(zlib EXCLUDE_FROM_ALL)

# Next, set ZLIB_INCLUDE_DIR and ZLIB_LIBRARY so that when find_package(ZLIB) is
# called, it is satisfied without actually running most of its search logic.
set(ZLIB_INCLUDE_DIR zlib_nonexistent_include_dir)
set(ZLIB_LIBRARY zlibstatic)

# Now add an empty ZLIB::ZLIB interface library to stop find_package(ZLIB) from
# creating one. libpng will still add the target to its list of link libraries,
# but since it's an empty target, it will have no effect.
add_library(ZLIB::ZLIB INTERFACE IMPORTED)

option(PNG_SHARED "Build shared lib" OFF)
option(PNG_STATIC "Build static lib" ON)
option(PNG_EXECUTABLES "Build libpng executables" OFF)
option(PNG_TESTS "Build libpng tests" OFF)
set(SKIP_INSTALL_ALL TRUE)

add_subdirectory(libpng EXCLUDE_FROM_ALL)
