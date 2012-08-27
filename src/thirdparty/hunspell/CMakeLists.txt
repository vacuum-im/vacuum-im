include("${CMAKE_SOURCE_DIR}/config.cmake")

project(hunspell CXX)
message(STATUS "Using bundled hunspell")

set(SOURCES affentry.cxx affixmgr.cxx csutil.cxx dictmgr.cxx filemgr.cxx hashmgr.cxx hunspell.cxx hunzip.cxx phonet.cxx replist.cxx suggestmgr.cxx)

add_definitions(-DHUNSPELL_STATIC)
 
add_library(hunspell STATIC ${SOURCES})
set_target_properties(hunspell PROPERTIES ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/src/libs"
	COMPILE_FLAGS "${CMAKE_STATIC_LIBRARY_C_FLAGS}"
	LINK_FLAGS "${CMAKE_STATIC_LIBRARY_C_FLAGS}")

IF(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" AND NOT MSVC)
	set_source_files_properties(${SOURCES} COMPILE_FLAGS "${CMAKE_STATIC_LIBRARY_C_FLAGS} -fPIC")
ENDIF(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" AND NOT MSVC) 
