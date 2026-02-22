# Include vendored Berry scripting language (source/berry/vendor/)
set(BERRY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/source/berry/vendor)

message(STATUS "Using vendored Berry from ${BERRY_DIR}")

file(GLOB BERRY_SOURCES ${BERRY_DIR}/*.c)

add_library(libberry STATIC ${BERRY_SOURCES})
target_include_directories(libberry PUBLIC ${BERRY_DIR})
target_compile_options(libberry PRIVATE -w)
