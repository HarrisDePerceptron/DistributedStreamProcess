cmake_minimum_required(VERSION 3.16)

# set the project name
project(DistributedTask VERSION 1.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


find_package(Threads REQUIRED)


find_package(nlohmann_json CONFIG REQUIRED)
find_path(UWEBSOCKETS_INCLUDE_DIRS "uwebsockets/App.h")
find_library(USOCKETS_LIB uSockets)
find_package(libuv CONFIG REQUIRED)
find_library(LIBUV_LIB uv_a)


find_package(ZLIB REQUIRED)

find_package(absl CONFIG REQUIRED)

find_package(cpr CONFIG REQUIRED)
find_package(PkgConfig)


find_package(redis++ CONFIG REQUIRED)

find_package(fmt CONFIG REQUIRED)


find_package(spdlog CONFIG REQUIRED)

add_subdirectory(src)
