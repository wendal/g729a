cmake_minimum_required(VERSION 3.16)
project(g729a VERSION 2.0 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

option(G729A_ITU_TEST_FORMAT "Use ITU test-vector bitstream format (164 bytes/frame)" OFF)

file(GLOB G729A_LIB_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")
list(REMOVE_ITEM G729A_LIB_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/coder.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/decoder.c"
)

add_library(g729a STATIC ${G729A_LIB_SOURCES})
target_include_directories(g729a
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src/interface"
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

if(MSVC)
    target_compile_options(g729a PRIVATE /W3)
else()
    target_compile_options(g729a PRIVATE -Wall -Wextra)
endif()

add_executable(coder src/coder.c)
target_link_libraries(coder PRIVATE g729a)

add_executable(decoder src/decoder.c)
target_link_libraries(decoder PRIVATE g729a)

add_executable(test_roundtrip tests/test_roundtrip.c)
target_link_libraries(test_roundtrip PRIVATE g729a)

if(G729A_ITU_TEST_FORMAT)
    target_compile_definitions(g729a PUBLIC CONTROL_OPT_ITU=1)
endif()
