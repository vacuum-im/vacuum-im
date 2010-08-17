include("${CMAKE_SOURCE_DIR}/config.cmake")

project(minizip C)

file(GLOB SOURCES "*.c")

include_directories("../zlib")

add_library(minizip STATIC ${SOURCES})
set_target_properties(minizip PROPERTIES ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/src/libs")
