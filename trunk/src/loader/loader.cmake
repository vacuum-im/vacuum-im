file(GLOB SOURCES "*.cpp")
file(GLOB UI "*.ui")
set(HEADERS "aboutbox.h"
		"pluginmanager.h"
		"setuppluginsdialog.h")

qt4_wrap_cpp(MOC_SOURCES ${HEADERS})
qt4_wrap_ui(UI_HEADERS ${UI})
qt4_add_resources(RC_SOURCES "loader.rc")
