include("${CMAKE_SOURCE_DIR}/src/make/config.cmake")

project(qtlockedfile CXX)

set(SOURCES "qtlockedfile.cpp")

set(QT_DONT_USE_QTGUI YES)
include(${QT_USE_FILE})

if (UNIX)
	set(SOURCES ${SOURCES} "qtlockedfile_unix.cpp")
elseif (WIN32)
	set(SOURCES ${SOURCES} "qtlockedfile_win.cpp")
endif (UNIX)

add_definitions(-DUNICODE)

add_library(qtlockedfile STATIC ${SOURCES})
target_link_libraries(qtlockedfile ${QT_LIBRARIES})
set_target_properties(qtlockedfile PROPERTIES ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/src/libs"
	COMPILE_FLAGS "${CMAKE_SHARED_LIBRARY_C_FLAGS}"
	LINK_FLAGS "${CMAKE_SHARED_LIBRARY_C_FLAGS}")
