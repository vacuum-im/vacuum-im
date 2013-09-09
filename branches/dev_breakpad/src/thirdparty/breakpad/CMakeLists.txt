include("${CMAKE_SOURCE_DIR}/src/make/config.cmake")

project(breakpad CXX)

add_definitions (-DUNICODE)

include_directories("${CMAKE_SOURCE_DIR}/src/thirdparty/breakpad")

if (APPLE)
	#set(SOURCES )
elseif (UNIX)
	#set(SOURCES )
elseif (WIN32)
	set(HEADERS "common/windows/string_utils-inl.h"
                "common/windows/guid_string.h"
                "client/windows/handler/exception_handler.h"
                "client/windows/common/ipc_protocol.h"
                "google_breakpad/common/minidump_format.h"
                "google_breakpad/common/breakpad_types.h"
                "client/windows/crash_generation/crash_generation_client.h"
                "processor/scoped_ptr.h")

	set(SOURCES "client/windows/handler/exception_handler.cc"
                "common/windows/string_utils.cc"
                "common/windows/guid_string.cc"
                "client/windows/crash_generation/crash_generation_client.cc")
endif (APPLE)

add_library(breakpad STATIC ${SOURCES})
set_target_properties(breakpad PROPERTIES ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/src/libs"
	COMPILE_FLAGS "${CMAKE_SHARED_LIBRARY_C_FLAGS}"
	LINK_FLAGS "${CMAKE_SHARED_LIBRARY_C_FLAGS}")
