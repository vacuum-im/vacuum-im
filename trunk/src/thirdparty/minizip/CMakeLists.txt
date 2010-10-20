include("${CMAKE_SOURCE_DIR}/config.cmake")

project(minizip C)

file(GLOB SOURCES "*.c")

include_directories("../zlib")

add_library(minizip STATIC ${SOURCES})
set_target_properties(minizip PROPERTIES ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/src/libs"
	COMPILE_FLAGS "${CMAKE_SHARED_LIBRARY_C_FLAGS}"
	LINK_FLAGS "${CMAKE_SHARED_LIBRARY_C_FLAGS}")

if (INSTALL_SDK)
	file(GLOB ALL_HEADERS "*.h")
	install(FILES ${ALL_HEADERS}
		DESTINATION "${INSTALL_INCLUDES}/thirdparty/minizip"
		COMPONENT sdk)
	if (WIN32)
		install(TARGETS minizip
			ARCHIVE DESTINATION "${INSTALL_LIBS}"
			COMPONENT sdk)
	endif (WIN32)
endif (INSTALL_SDK)
