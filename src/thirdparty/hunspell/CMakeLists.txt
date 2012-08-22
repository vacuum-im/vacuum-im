include("${CMAKE_SOURCE_DIR}/config.cmake")

project(hunspell CXX)

set(SOURCES affentry.cxx affixmgr.cxx csutil.cxx dictmgr.cxx filemgr.cxx hashmgr.cxx hunspell.cxx hunzip.cxx phonet.cxx replist.cxx suggestmgr.cxx)

add_definitions(-DHUNSPELL_STATIC)
 
add_library(hunspell STATIC ${SOURCES})
set_target_properties(hunspell PROPERTIES ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/src/libs"
	COMPILE_FLAGS "${CMAKE_SHARED_LIBRARY_C_FLAGS}"
	LINK_FLAGS "${CMAKE_SHARED_LIBRARY_C_FLAGS}")
