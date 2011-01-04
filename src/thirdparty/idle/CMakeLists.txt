include("${CMAKE_SOURCE_DIR}/config.cmake")

project(idle CXX)

set(SOURCES "idle.cpp")

include(${QT_USE_FILE})

if (APPLE)
	set(SOURCES ${SOURCES} "idle_mac.cpp")
elseif (UNIX)
	set(SOURCES ${SOURCES} "idle_x11.cpp")
	if (X11_Xscreensaver_FOUND)
		add_definitions(-DHAVE_XSS)
		include_directories(AFTER "${X11_Xscreensaver_INCLUDE_PATH}")
	endif (X11_Xscreensaver_FOUND)
elseif (WIN32)
	set(SOURCES ${SOURCES} "idle_win.cpp")
endif (APPLE)

qt4_wrap_cpp(MOC_SOURCES "idle.h")

add_definitions(-DUNICODE)

add_library(idle STATIC ${SOURCES} ${MOC_SOURCES})
target_link_libraries(idle ${QT_LIBRARIES})
set_target_properties(idle PROPERTIES ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/src/libs"
	COMPILE_FLAGS "${CMAKE_SHARED_LIBRARY_C_FLAGS}"
	LINK_FLAGS "${CMAKE_SHARED_LIBRARY_C_FLAGS}")
