set(SOURCES "main.cpp"
			"aboutbox.cpp"
			"pluginmanager.cpp"
			"setuppluginsdialog.cpp")

set(UIS "aboutbox.ui"
		"setuppluginsdialog.ui")

set(HEADERS "aboutbox.h"
			"pluginmanager.h"
			"setuppluginsdialog.h")

qt5_wrap_cpp(MOC_SOURCES ${HEADERS})
qt5_wrap_ui(UI_HEADERS ${UIS})

if (NOT MSVC)
	qt5_add_resources(RC_SOURCES "loader.rc")
endif (NOT MSVC)
