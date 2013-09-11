set(SOURCES "main.cpp"
			"aboutbox.cpp"
			"pluginmanager.cpp"
			"setuppluginsdialog.cpp")
set(HEADERS "aboutbox.h"
			"pluginmanager.h"
			"setuppluginsdialog.h")
file(GLOB UIS "*.ui")

#Google Breakpad
if(NOT (BREAKPAD_DEV_DIR STREQUAL ""))
	add_definitions(-DWITH_BREAKPAD)
	set(SOURCES ${SOURCES} "crashhandler.cpp")
	set(HEADERS ${HEADERS} "crashhandler.h")
endif(NOT (BREAKPAD_DEV_DIR STREQUAL ""))

qt4_wrap_cpp(MOC_SOURCES ${HEADERS})
qt4_wrap_ui(UI_HEADERS ${UIS})
if (NOT MSVC)
	qt4_add_resources(RC_SOURCES "loader.rc")
endif (NOT MSVC)
