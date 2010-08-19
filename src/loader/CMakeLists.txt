include("${CMAKE_SOURCE_DIR}/config.cmake")
include("loader.cmake")

project(loader CXX)

link_directories("${CMAKE_BINARY_DIR}/src/libs")
include_directories("${CMAKE_SOURCE_DIR}/src" "${CMAKE_BINARY_DIR}/src/loader")
set(QT_USE_QTXML YES)
include(${QT_USE_FILE})

execute_process(COMMAND svnversion -n -c "${CMAKE_SOURCE_DIR}"
	OUTPUT_VARIABLE SVN_REVISION)
if (NOT (SVN_REVISION STREQUAL ""))
	file(REMOVE "svninfo.h")
	file(WRITE "svninfo.h" "#define SVN_REVISION \"${SVN_REVISION}\"\n")
	add_definitions(-DSVNINFO)
endif (NOT (SVN_REVISION STREQUAL ""))

add_translations(TRANSLATIONS "${TARGET_LOADER}" ${SOURCES} ${UIS})
add_executable(${TARGET_LOADER} ${SOURCES} ${MOC_SOURCES} ${UI_HEADERS} ${TRANSLATIONS})
set_target_properties(${TARGET_LOADER} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
target_link_libraries(${TARGET_LOADER} ${TARGET_UTILS} ${QT_LIBRARIES})

install(TARGETS ${TARGET_LOADER} RUNTIME
	DESTINATION "${INSTALL_BINS}")
if (UNIX AND NOT APPLE)
	install(FILES "${CMAKE_SOURCE_DIR}/resources/menuicons/shared/vacuum.png"
		DESTINATION "${INSTALL_RES_DIR}/pixmaps")
	install(FILES "${CMAKE_SOURCE_DIR}/src/packages/linux/vacuum.desktop"
		DESTINATION "${INSTALL_RES_DIR}/applications")
endif (UNIX AND NOT APPLE)

