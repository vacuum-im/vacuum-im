include("${CMAKE_SOURCE_DIR}/config.cmake")
include("utils.cmake")

project(${TARGET_UTILS})

set(QT_USE_QTXML YES)
include(${QT_USE_FILE})

add_definitions("-DUTILS_DLL")
link_directories("${CMAKE_BINARY_DIR}/libs")

add_translations(TRANSLATIONS "${TARGET_UTILS}" ${SOURCES})
add_library(${TARGET_UTILS} SHARED ${SOURCES} ${MOC_SOURCES} ${TRANSLATIONS})

if (UNIX AND NOT APPLE)
	set(ADD_LIBS idn minizip "-lz")
else (UNIX AND NOT APPLE)
	set(ADD_LIBS idn minizip zlib)
endif (UNIX AND NOT APPLE)

target_link_libraries(${TARGET_UTILS} LINK_INTERFACE_LIBRARIES ${QT_LIBRARIES})
target_link_libraries(${TARGET_UTILS} ${QT_LIBRARIES} ${ADD_LIBS})

if (WIN32)
	set_target_properties(${TARGET_UTILS} PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
else (WIN32)
	set_target_properties(${TARGET_UTILS} PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/src/libs")
endif (WIN32)

