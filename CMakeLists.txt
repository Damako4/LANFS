cmake_minimum_required(VERSION 3.10.0)
project(LANFS VERSION 0.1.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)

find_package(OpenSSL REQUIRED)

add_library(lanfs_core STATIC
    src/common/ProtocolHandler.cpp    
)

target_include_directories(lanfs_core PUBLIC include/common)
target_link_libraries(lanfs_core PUBLIC OpenSSL::SSL OpenSSL::Crypto)

add_executable(lanfs_server
    src/server/main.cpp
    src/server/Application.cpp
)

target_include_directories(lanfs_server PRIVATE include/server)
target_link_libraries(lanfs_server PRIVATE lanfs_core)

add_executable(lanfs_client
    src/client/main.cpp
)

target_include_directories(lanfs_server PRIVATE include/client)
target_link_libraries(lanfs_client PRIVATE lanfs_core)