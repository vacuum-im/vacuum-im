project(birthdayreminder)

set(PLUGIN_NAME "birthdayreminder")
set(PLUGIN_DISPLAY_NAME "Birthday Reminder")
set(PLUGIN_DEPENDENCIES vcard) # used only in CPack

include("birthdayreminder.cmake")
include("${CMAKE_SOURCE_DIR}/src/plugins/plugins.cmake")
