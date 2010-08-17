include("${CMAKE_SOURCE_DIR}/config.cmake")

project(jdns C CXX)

set(QT_USE_QTNETWORK YES)
set(QT_NOUSE_QTGUI YES)
include(${QT_USE_FILE})

include_directories(${CMAKE_CURRENT_BINARY_DIR})

include("jdns.cmake")

qt4_wrap_cpp(MOC_SOURCES ${HEADERS})

add_library(jdns STATIC ${SOURCES} ${MOC_SOURCES} "qjdns.moc")
target_link_libraries(jdns ${QT_LIBRARIES})
target_link_libraries(jdns LINK_INTERFACE_LIBRARIES)
set_target_properties(jdns PROPERTIES
	ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/src/libs"
	COMPILE_FLAGS "-w")
