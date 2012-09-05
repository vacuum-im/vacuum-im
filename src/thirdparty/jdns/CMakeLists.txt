include("${CMAKE_SOURCE_DIR}/src/make/config.cmake")

project(jdns C CXX)

set(QT_USE_QTNETWORK YES)
set(QT_NOUSE_QTGUI YES)
include(${QT_USE_FILE})

include_directories(${CMAKE_CURRENT_BINARY_DIR})

include("jdns.cmake")

qt4_wrap_cpp(MOC_SOURCES ${HEADERS})

add_library(jdns STATIC ${SOURCES} ${MOC_SOURCES} "${CMAKE_CURRENT_BINARY_DIR}/qjdns.moc")
target_link_libraries(jdns ${QT_LIBRARIES} ${LIBS})
target_link_libraries(jdns LINK_INTERFACE_LIBRARIES)
set_target_properties(jdns PROPERTIES
	ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/src/libs"
	COMPILE_FLAGS "-w ${CMAKE_SHARED_LIBRARY_C_FLAGS}"
	LINK_FLAGS "${CMAKE_SHARED_LIBRARY_C_FLAGS}")
