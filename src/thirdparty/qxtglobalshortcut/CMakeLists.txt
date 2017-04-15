include("${CMAKE_SOURCE_DIR}/src/make/config.cmake")

project(qxtglobalshortcut CXX)

if (UNIX AND NOT APPLE)
	find_package (Qt5X11Extras REQUIRED)
endif (UNIX AND NOT APPLE)

set(SOURCES "qxtglobal.cpp" "qxtglobalshortcut.cpp")
set(HEADERS "qxtglobalshortcut.h")

if (APPLE)
	set(SOURCES ${SOURCES} "qxtglobalshortcut_mac.cpp")
elseif (HAIKU)
	set(SOURCES ${SOURCES} "qxtglobalshortcut_haiku.cpp")
elseif (UNIX)
	set(SOURCES ${SOURCES} "qxtglobalshortcut_x11.cpp")
	include(FindX11)
	include_directories(AFTER ${X11_INCLUDE_DIR})
elseif (WIN32)
	set(SOURCES ${SOURCES} "qxtglobalshortcut_win.cpp")
endif (APPLE)

add_definitions(-DUNICODE -DQXT_STATIC)

qt5_wrap_cpp(MOC_SOURCES ${HEADERS})

add_library(qxtglobalshortcut STATIC ${SOURCES} ${MOC_SOURCES})

if (UNIX AND NOT APPLE)
    target_link_libraries(qxtglobalshortcut Qt5::Widgets Qt5::X11Extras)
else(UNIX AND NOT APPLE)
    target_link_libraries(qxtglobalshortcut Qt5::Widgets)
endif (UNIX AND NOT APPLE)

set_target_properties(qxtglobalshortcut PROPERTIES ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/src/libs"
	COMPILE_FLAGS "${CMAKE_SHARED_LIBRARY_C_FLAGS}"
	LINK_FLAGS "${CMAKE_SHARED_LIBRARY_C_FLAGS}")
