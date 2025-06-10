cmake_minimum_required(VERSION 3.10)
project(pomelo-udp-quickjs VERSION 1.0.0 LANGUAGES C)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(POMELO_BUILD_TESTS OFF)
set(POMELO_BUILD_EXAMPLES OFF)
set(POMELO_BUILD_GENERATOR OFF)
add_subdirectory(deps/pomelo-udp-native)

add_subdirectory(deps/quickjs)

# Add source files
set(SOURCES
    src/module.c
)

# Add library
add_library(${PROJECT_NAME} SHARED ${SOURCES})

# Link dependencies
target_link_libraries(${PROJECT_NAME} PRIVATE
    qjs
    pomelo-base
    pomelo-protocol
    pomelo-delivery
    pomelo-platform-uv
    pomelo-adapter-default
    pomelo-crypto
    pomelo-utils
    pomelo-api
)
