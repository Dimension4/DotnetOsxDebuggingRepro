cmake_minimum_required(VERSION 3.20)

project(NativeHost)

set(CMAKE_CXX_STANDARD 17)
add_compile_definitions (_UNICODE UNICODE)

add_library (NativeHost MODULE main.cpp)

set_target_properties(
    NativeHost
    PROPERTIES
        SUFFIX ".so"
    )

if (APPLE)
    set_target_properties(
        NativeHost
        PROPERTIES
            PREFIX ""
            SUFFIX ".bundle"
    )
endif()