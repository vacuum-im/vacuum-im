project(messagearchiver)

set(PLUGIN_NAME "messagearchiver")
set(PLUGIN_DISPLAY_NAME "Message archiver")
set(PLUGIN_DEPENDENCIES xmppstreams stanzaprocessor) # used only in CPack

include("messagearchiver.cmake")
include("${CMAKE_SOURCE_DIR}/src/plugins/plugins.cmake")

if (IS_ENABLED)
	find_package(Qt5PrintSupport REQUIRED)
	find_package(Qt5Sql REQUIRED)
	qt5_use_modules(${PLUGIN_NAME} Sql PrintSupport)
endif (IS_ENABLED)