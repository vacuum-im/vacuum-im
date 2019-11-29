include("${CMAKE_SOURCE_DIR}/src/make/config.cmake")

project(zlib C)

file(GLOB SOURCES "*.c")

add_library(zlib STATIC ${SOURCES})
set_target_properties(zlib PROPERTIES ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/src/libs"
	COMPILE_FLAGS "${CMAKE_SHARED_LIBRARY_C_FLAGS}"
	LINK_FLAGS "${CMAKE_SHARED_LIBRARY_C_FLAGS}")

IF(MSVC)
        add_definitions(-D_CRT_SECURE_NO_WARNINGS)
ENDIF(MSVC)
