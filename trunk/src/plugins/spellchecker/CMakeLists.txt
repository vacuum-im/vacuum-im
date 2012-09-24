project(spellchecker)

set(PLUGIN_NAME "spellchecker")
set(PLUGIN_DISPLAY_NAME "Spellchecker")
set(PLUGIN_DEPENDENCIES messagewidgets) # used only in CPack 

if (UNIX AND NOT APPLE)
	set(SPELLCHECKER_BACKEND "ENCHANT" CACHE STRING "Name of engine that will be used in Spellchecker plugin")
else (UNIX AND NOT APPLE)
	set(SPELLCHECKER_BACKEND "HUNSPELL" CACHE STRING "Name of engine that will be used in Spellchecker plugin")
endif (UNIX AND NOT APPLE)

string(TOUPPER ${SPELLCHECKER_BACKEND} SPELLCHECKER_BACKEND)

include("spellchecker.cmake")

if (SPELLCHECKER_BACKEND STREQUAL "ASPELL")
	include("aspellchecker.cmake")
elseif (SPELLCHECKER_BACKEND STREQUAL "ENCHANT")
	include("enchantchecker.cmake")
elseif (SPELLCHECKER_BACKEND STREQUAL "MACSPELL")
	include("macspellchecker.cmake")
elseif (SPELLCHECKER_BACKEND STREQUAL "HUNSPELL")
	include("hunspellchecker.cmake")
else (SPELLCHECKER_BACKEND STREQUAL "ASPELL")
	message(FATAL_ERROR "Please specify correct backend name, only aspell, enchant, hunspell, macspell supported")
endif (SPELLCHECKER_BACKEND STREQUAL "ASPELL")

include("${CMAKE_SOURCE_DIR}/src/plugins/plugins.cmake")
