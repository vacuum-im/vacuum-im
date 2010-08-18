file(GLOB SOURCES "*.cpp")
file(GLOB UIS "*.ui")
set(HEADERS "aboutbox.h"
		"pluginmanager.h"
		"setuppluginsdialog.h")

qt4_wrap_cpp(MOC_SOURCES ${HEADERS})
qt4_wrap_ui(UI_HEADERS ${UIS})
qt4_add_resources(RC_SOURCES "loader.rc")
