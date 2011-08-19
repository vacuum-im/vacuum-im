include("${CMAKE_SOURCE_DIR}/config.cmake")

project(qxtglobalshortcut CXX)

set(SOURCES "qxtglobal.cpp" "qxtglobalshortcut.cpp")

include(${QT_USE_FILE})

if (APPLE)
	set(SOURCES ${SOURCES} "qxtglobalshortcut_mac.cpp")
elseif (UNIX)
	set(SOURCES ${SOURCES} "qxtglobalshortcut_x11.cpp")
	include(FindX11)
	include_directories(AFTER ${X11_INCLUDE_DIR})
elseif (WIN32)
	set(SOURCES ${SOURCES} "qxtglobalshortcut_win.cpp")
endif (APPLE)

qt4_wrap_cpp(MOC_SOURCES "qxtglobalshortcut.h")

add_definitions(-DUNICODE)
add_definitions(-DQXT_STATIC)

add_library(qxtglobalshortcut STATIC ${SOURCES} ${MOC_SOURCES})
target_link_libraries(qxtglobalshortcut ${QT_LIBRARIES})
set_target_properties(qxtglobalshortcut PROPERTIES ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/src/libs"
	COMPILE_FLAGS "${CMAKE_SHARED_LIBRARY_C_FLAGS}"
	LINK_FLAGS "${CMAKE_SHARED_LIBRARY_C_FLAGS}")
