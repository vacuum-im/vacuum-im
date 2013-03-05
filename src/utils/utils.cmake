file(GLOB SOURCES "*.cpp")

set(HEADERS "action.h"
            "filestorage.h"
            "iconstorage.h"
            "menu.h"
            "menubarchanger.h"
            "options.h"
            "shortcuts.h"
            "statusbarchanger.h"
            "systemmanager.h"
            "toolbarchanger.h"
            "animatedtextbrowser.h")

qt4_wrap_cpp(MOC_SOURCES ${HEADERS})
