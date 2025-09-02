cmake_minimum_required(VERSION 3.10)
project(pomelo-udp-quickjs VERSION 1.0.0 LANGUAGES C)


# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(POMELO_QJS_BUILD_STANDALONE "Build standalone library" OFF)

set(POMELO_BUILD_TESTS OFF)
set(POMELO_BUILD_EXAMPLES OFF)
set(POMELO_BUILD_GENERATOR OFF)

add_subdirectory(deps/pomelo-udp-native)
add_subdirectory(deps/quickjs)

# Add system include directories for deps to suppress warnings in their headers
include_directories(SYSTEM 
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/quickjs
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/pomelo-udp-native
)

# Set warning flags
if(MSVC)
    set(POMELO_QJS_COMPILE_FLAGS /W4 /WX /Wv:18)
    set(POMELO_QJS_LINK_FLAGS)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
else()
    set(POMELO_QJS_COMPILE_FLAGS -Wall -Wpedantic -Wextra -Werror)
    set(POMELO_QJS_LINK_FLAGS -ldl)
endif()

set(POMELO_QJS pomelo-qjs)
set(POMELO_QJS_CORE ${POMELO_QJS}-core)
set(POMELO_QJS_RUNTIME ${POMELO_QJS}-runtime)
set(POMELO_QJS_RUNTIME_UV ${POMELO_QJS}-runtime-uv)
set(POMELO_QJS_STD ${POMELO_QJS}-std)
set(POMELO_QJS_STANDALONE ${POMELO_QJS}-standalone)


set(POMELO_QJS_INCLUDE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
set(POMELO_INCLUDE
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/pomelo-udp-native/include
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/pomelo-udp-native/src
)
set(UV_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/deps/pomelo-udp-native/deps/libuv/include)
set(QJS_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/deps/quickjs)

# Build core library
add_library(${POMELO_QJS_CORE} STATIC EXCLUDE_FROM_ALL
    src/core/channel.c
    src/core/channel.h
    src/core/context.c
    src/core/context.h
    src/core/core.c
    src/core/core.h
    src/core/enums.c
    src/core/enums.h
    src/core/functions.c
    src/core/functions.h
    src/core/message.c
    src/core/message.h
    src/core/plugin.c
    src/core/plugin.h
    src/core/session.c
    src/core/session.h
    src/core/socket.c
    src/core/socket.h
    src/core/token.c
    src/core/token.h
)
target_include_directories(${POMELO_QJS_CORE}
    PRIVATE ${POMELO_INCLUDE} ${POMELO_QJS_INCLUDE}
    PUBLIC ${QJS_INCLUDE}
)
target_link_options(${POMELO_QJS_CORE} PUBLIC ${POMELO_QJS_LINK_FLAGS})
target_compile_options(${POMELO_QJS_CORE} PRIVATE ${POMELO_QJS_COMPILE_FLAGS})


# Build std library
add_library(${POMELO_QJS_STD} STATIC EXCLUDE_FROM_ALL
    src/std/std.c
    src/std/std.h
    src/std/timer.c
    src/std/timer.h
    src/std/console.c
    src/std/console.h
)
target_include_directories(${POMELO_QJS_STD}
    PUBLIC ${POMELO_QJS_INCLUDE} ${POMELO_INCLUDE} ${QJS_INCLUDE}
)
target_link_options(${POMELO_QJS_STD} PUBLIC ${POMELO_QJS_LINK_FLAGS})
target_compile_options(${POMELO_QJS_STD} PRIVATE ${POMELO_QJS_COMPILE_FLAGS})


# Build runtime library
add_library(${POMELO_QJS_RUNTIME} STATIC EXCLUDE_FROM_ALL
    src/runtime/runtime.c
    src/runtime/runtime.h
)
target_include_directories(${POMELO_QJS_RUNTIME}
    PUBLIC ${POMELO_QJS_INCLUDE} ${QJS_INCLUDE} ${POMELO_INCLUDE}
)
target_link_options(${POMELO_QJS_RUNTIME} PUBLIC ${POMELO_QJS_LINK_FLAGS})
target_compile_options(${POMELO_QJS_RUNTIME} PRIVATE ${POMELO_QJS_COMPILE_FLAGS})

add_library(${POMELO_QJS_RUNTIME_UV} STATIC EXCLUDE_FROM_ALL
    src/runtime/runtime-uv.c
    src/runtime/runtime-uv.h
)
target_include_directories(${POMELO_QJS_RUNTIME_UV}
    PUBLIC ${POMELO_QJS_INCLUDE} ${QJS_INCLUDE} ${POMELO_INCLUDE}
    PRIVATE ${UV_INCLUDE}
)
target_link_options(${POMELO_QJS_RUNTIME_UV} PUBLIC ${POMELO_QJS_LINK_FLAGS})
target_compile_options(${POMELO_QJS_RUNTIME_UV} PRIVATE ${POMELO_QJS_COMPILE_FLAGS})


# Build standalone executable
if (POMELO_QJS_BUILD_STANDALONE)
    add_executable(${POMELO_QJS_STANDALONE} src/standalone/standalone.c)
    target_link_libraries(${POMELO_QJS_STANDALONE} PRIVATE
        ${POMELO_QJS_CORE}
        ${POMELO_QJS_RUNTIME}
        ${POMELO_QJS_RUNTIME_UV}
        ${POMELO_QJS_STD}
        qjs
        uv_a
        pomelo-base
        pomelo-protocol
        pomelo-delivery
        pomelo-platform-uv
        pomelo-adapter-default
        pomelo-crypto
        pomelo-utils
        pomelo-api
        sodium
    )
    target_compile_options(${POMELO_QJS_STANDALONE} PRIVATE ${POMELO_QJS_COMPILE_FLAGS})
endif()
