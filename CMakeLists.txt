cmake_minimum_required(VERSION 3.13)
project(TransDB LANGUAGES C CXX)

# The code targets C++11, C99, Python 2.7 and Intel TBB 4.2, exactly as the
# hand-written Makefile / Visual Studio 2010 / Xcode projects did.
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

option(TRANSDB_BUILD_TESTER      "Build the load-generating test client"        ON)
option(TRANSDB_BUNDLED_ZLIB      "Compile the vendored zlib in src/shared/zlib" ON)
option(TRANSDB_SCALABLE_ALLOCATOR "Route allocations through tbbmalloc"         ON)
set(TRANSDB_SOCKET_BACKEND "" CACHE STRING
    "Socket backend: IOCP, KEVENT, EPOLL or SELECT (empty picks one per platform)")
set(TRANSDB_TBB_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/tbb" CACHE PATH
    "Intel TBB prefix; the in-tree copy ships headers and macOS dylibs")

# crc32_sse.c uses the SSE4.2 crc32 instruction through inline asm (and cpuid.h
# is x86-only), so the storage checksum will not build elsewhere.
if(NOT CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64|i[3-6]86")
    message(WARNING
        "TransDB contains x86-only SSE4.2 code (src/shared/clib/Crc32/crc32_sse.c); "
        "the build is expected to fail on ${CMAKE_SYSTEM_PROCESSOR}.")
endif()

# --------------------------------------------------------------------------
# Socket backend
# --------------------------------------------------------------------------
if(NOT TRANSDB_SOCKET_BACKEND)
    if(WIN32)
        set(TRANSDB_SOCKET_BACKEND IOCP)
    elseif(APPLE OR CMAKE_SYSTEM_NAME MATCHES "BSD|DragonFly")
        set(TRANSDB_SOCKET_BACKEND KEVENT)
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        set(TRANSDB_SOCKET_BACKEND EPOLL)
    else()
        set(TRANSDB_SOCKET_BACKEND SELECT)
    endif()
endif()

if(NOT TRANSDB_SOCKET_BACKEND MATCHES "^(IOCP|KEVENT|EPOLL|SELECT)$")
    message(FATAL_ERROR "TRANSDB_SOCKET_BACKEND must be IOCP, KEVENT, EPOLL or SELECT")
endif()
message(STATUS "TransDB socket backend: ${TRANSDB_SOCKET_BACKEND}")

# --------------------------------------------------------------------------
# Dependencies
# --------------------------------------------------------------------------
find_package(Threads REQUIRED)

# Intel TBB 4.2 predates the CMake config packages that ship with oneTBB, so
# look it up by hand and prefer the in-tree copy.
find_path(TBB_INCLUDE_DIR
    NAMES tbb/concurrent_hash_map.h
    HINTS "${TRANSDB_TBB_ROOT}/include")
find_library(TBB_LIBRARY
    NAMES tbb
    HINTS "${TRANSDB_TBB_ROOT}/lib")
if(NOT TBB_INCLUDE_DIR OR NOT TBB_LIBRARY)
    message(FATAL_ERROR
        "Intel TBB not found. Set -DTRANSDB_TBB_ROOT=<prefix>, or install libtbb-dev. "
        "The bundled tbb/lib only holds macOS dylibs.")
endif()

add_library(TransDB::tbb UNKNOWN IMPORTED)
set_target_properties(TransDB::tbb PROPERTIES
    IMPORTED_LOCATION "${TBB_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${TBB_INCLUDE_DIR}")

if(TRANSDB_SCALABLE_ALLOCATOR)
    find_library(TBBMALLOC_LIBRARY
        NAMES tbbmalloc
        HINTS "${TRANSDB_TBB_ROOT}/lib")
    if(NOT TBBMALLOC_LIBRARY)
        message(FATAL_ERROR
            "tbbmalloc not found; configure with -DTRANSDB_SCALABLE_ALLOCATOR=OFF to use the system allocator.")
    endif()
    add_library(TransDB::tbbmalloc UNKNOWN IMPORTED)
    set_target_properties(TransDB::tbbmalloc PROPERTIES
        IMPORTED_LOCATION "${TBBMALLOC_LIBRARY}")
endif()

# The embedded interpreter uses the Python 2 C API (Py_InitModule, PyString_*),
# so Python 3 will not substitute.
find_package(Python2 COMPONENTS Development)
if(NOT Python2_FOUND)
    message(FATAL_ERROR
        "Python 2 development headers not found. Point CMake at them with "
        "-DPython2_ROOT_DIR=<prefix>; src/console/PythonInterface.cpp is Python 2 only.")
endif()

if(NOT TRANSDB_BUNDLED_ZLIB)
    find_package(ZLIB REQUIRED)
endif()

# --------------------------------------------------------------------------
# zlib (vendored 1.2.7)
# --------------------------------------------------------------------------
# src/shared/CommonFunctions.h includes "zlib/zlib.h" by relative path, so the
# vendored headers are used either way; building the vendored sources too keeps
# headers and object code from drifting apart.
if(TRANSDB_BUNDLED_ZLIB)
    file(GLOB TRANSDB_ZLIB_SOURCES CONFIGURE_DEPENDS "src/shared/zlib/*.c")
    add_library(transdb_zlib STATIC ${TRANSDB_ZLIB_SOURCES})
    target_include_directories(transdb_zlib PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src/shared/zlib")
    set_target_properties(transdb_zlib PROPERTIES POSITION_INDEPENDENT_CODE ON)
    if(MSVC)
        target_compile_definitions(transdb_zlib PRIVATE _CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_WARNINGS)
    endif()
    set(TRANSDB_ZLIB_TARGET transdb_zlib)
else()
    set(TRANSDB_ZLIB_TARGET ZLIB::ZLIB)
endif()

# --------------------------------------------------------------------------
# shared support library
# --------------------------------------------------------------------------
# Globbed to match the Makefile, which discovers sources with `find`.
file(GLOB_RECURSE TRANSDB_SHARED_SOURCES CONFIGURE_DEPENDS
    "src/shared/*.cpp"
    "src/shared/*.c")
list(FILTER TRANSDB_SHARED_SOURCES EXCLUDE REGEX "/src/shared/zlib/")
if(NOT WIN32)
    # Wraps dbghelp.dll; only the Windows crash handler calls it.
    list(FILTER TRANSDB_SHARED_SOURCES EXCLUDE REGEX "/StackWalker\\.cpp$")
endif()

add_library(transdb_shared STATIC ${TRANSDB_SHARED_SOURCES})
target_include_directories(transdb_shared PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src/shared")
target_compile_definitions(transdb_shared PUBLIC
    CONFIG_USE_${TRANSDB_SOCKET_BACKEND}
    $<$<CONFIG:Debug>:DEBUG>)
target_link_libraries(transdb_shared
    PUBLIC TransDB::tbb ${TRANSDB_ZLIB_TARGET} Threads::Threads)

if(TRANSDB_SCALABLE_ALLOCATOR)
    target_compile_definitions(transdb_shared PUBLIC INTEL_SCALABLE_ALLOCATOR)
    target_link_libraries(transdb_shared PUBLIC TransDB::tbbmalloc)
endif()

if(WIN32)
    target_compile_definitions(transdb_shared PUBLIC WIN32 _CRT_SECURE_NO_WARNINGS)
    target_link_libraries(transdb_shared PUBLIC ws2_32 dbghelp)
endif()

# --------------------------------------------------------------------------
# transdb — the server
# --------------------------------------------------------------------------
# ServiceWin32.cpp is wrapped in #ifdef WIN32 and compiles to nothing elsewhere.
file(GLOB TRANSDB_CONSOLE_SOURCES CONFIGURE_DEPENDS
    "src/console/*.cpp"
    "src/console/*.c")

add_executable(transdb ${TRANSDB_CONSOLE_SOURCES})
target_link_libraries(transdb PRIVATE transdb_shared Python2::Python)

# --------------------------------------------------------------------------
# transdb_tester — load generator
# --------------------------------------------------------------------------
if(TRANSDB_BUILD_TESTER)
    file(GLOB TRANSDB_TESTER_SOURCES CONFIGURE_DEPENDS
        "src/tester/*.cpp"
        "src/tester/*.c")
    add_executable(transdb_tester ${TRANSDB_TESTER_SOURCES})
    target_link_libraries(transdb_tester PRIVATE transdb_shared)
endif()

# --------------------------------------------------------------------------
# install
# --------------------------------------------------------------------------
include(GNUInstallDirs)
install(TARGETS transdb RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
if(TRANSDB_BUILD_TESTER)
    install(TARGETS transdb_tester RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
endif()
install(FILES cfg/transdb.conf DESTINATION "${CMAKE_INSTALL_SYSCONFDIR}")
