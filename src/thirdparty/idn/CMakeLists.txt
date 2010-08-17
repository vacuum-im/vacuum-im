include("${CMAKE_SOURCE_DIR}/config.cmake")

project(idn C)

file(GLOB SOURCES "*.c")

add_library(idn STATIC ${SOURCES})
set_target_properties(idn PROPERTIES ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/src/libs")
