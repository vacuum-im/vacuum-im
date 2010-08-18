include("${CMAKE_SOURCE_DIR}/config.cmake")

project(zlib C)

file(GLOB SOURCES "*.c")

add_library(zlib STATIC ${SOURCES})
set_target_properties(zlib PROPERTIES ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/src/libs")
