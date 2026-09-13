cmake_minimum_required(VERSION 3.10.0)
project(LANFS VERSION 0.1.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

# Optional: This suppresses that CMP0167 FindBoost warning 
# caused by the msgpack-cxx configuration in newer CMake versions.
if(POLICY CMP0167)
    cmake_policy(SET CMP0167 OLD)
endif()

find_package(OpenSSL REQUIRED)
find_package(msgpack-cxx CONFIG REQUIRED)

find_path(LIBRSYNC_INCLUDE_DIR NAMES librsync.h)
find_library(LIBRSYNC_LIBRARY NAMES rsync librsync)

if(NOT LIBRSYNC_INCLUDE_DIR OR NOT LIBRSYNC_LIBRARY)
    message(FATAL_ERROR "librsync not found. include=${LIBRSYNC_INCLUDE_DIR} lib=${LIBRSYNC_LIBRARY}")
endif()

message(STATUS "librsync include: ${LIBRSYNC_INCLUDE_DIR}")
message(STATUS "librsync lib: ${LIBRSYNC_LIBRARY}")

find_path(EFSW_INCLUDE_DIR NAMES efsw/efsw.hpp)
find_library(EFSW_LIBRARY NAMES efsw-debug)

if(NOT EFSW_INCLUDE_DIR OR NOT EFSW_LIBRARY)
message(FATAL_ERROR "efsw not found. include=${EFSW_INCLUDE_DIR} lib=${EFSW_LIBRARY}")
endif()

message(STATUS "efsw include: ${EFSW_INCLUDE_DIR}")
message(STATUS "efsw lib: ${EFSW_LIBRARY}")

add_library(lanfs_core STATIC
    src/common/ProtocolHandler.cpp
    src/common/Tools.cpp
    src/common/FileHandler.cpp
)

target_include_directories(lanfs_core PUBLIC 
    include/common 
    ${LIBRSYNC_INCLUDE_DIR}
)

target_link_libraries(lanfs_core PUBLIC 
    OpenSSL::SSL 
    OpenSSL::Crypto 
    msgpack-cxx 
    ${LIBRSYNC_LIBRARY}
)

target_compile_definitions(lanfs_core PUBLIC $<$<CONFIG:Debug>:DEBUG>)

add_executable(lanfs_server
    src/server/main.cpp
    src/server/ServerApplication.cpp
)

target_include_directories(lanfs_server PRIVATE include/server)
target_link_libraries(lanfs_server PRIVATE lanfs_core)

add_executable(lanfs_client
    src/client/main.cpp
    src/client/ClientApplication.cpp
    src/client/UpdateListener.cpp
    src/client/FileEventQueue.cpp
)

target_include_directories(lanfs_client PRIVATE include/client ${EFSW_INCLUDE_DIR})
target_link_libraries(lanfs_client PRIVATE lanfs_core ${EFSW_LIBRARY})