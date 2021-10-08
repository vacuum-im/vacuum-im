project(messagearchiver)

set(QT_USE_QTSQL YES)

set(PLUGIN_NAME "messagearchiver")
set(PLUGIN_DISPLAY_NAME "Message archiver")
set(PLUGIN_DEPENDENCIES xmppstreams stanzaprocessor) # used only in CPack

include("messagearchiver.cmake")
include("${CMAKE_SOURCE_DIR}/src/plugins/plugins.cmake")

if (IS_ENABLED)
        find_package(Qt6PrintSupport REQUIRED)
        find_package(Qt6Sql REQUIRED)
        target_link_libraries(${PLUGIN_NAME} Qt6::PrintSupport Qt6::Sql)
endif (IS_ENABLED)
