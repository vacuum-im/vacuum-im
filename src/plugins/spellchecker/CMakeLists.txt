project(spellchecker)

set(PLUGIN_NAME "spellchecker")
set(PLUGIN_DISPLAY_NAME "Spellchecker")
set(PLUGIN_DEPENDENCIES messagewidgets) # used only in CPack 

set(SPELLCHECKER_BACKEND "HUNSPELL" CACHE STRING "Name of engine that will be used in Spellchecker plugin")
string(TOUPPER ${SPELLCHECKER_BACKEND} SPELLCHECKER_BACKEND)

include("spellchecker.cmake")

if (SPELLCHECKER_BACKEND STREQUAL "ASPELL")
	include("aspellchecker.cmake")
elseif (SPELLCHECKER_BACKEND STREQUAL "ENCHANT")
	include("enchantchecker.cmake")
elseif (SPELLCHECKER_BACKEND STREQUAL "MACSPELL")
	include("macspellchecker.cmake")
else (SPELLCHECKER_BACKEND STREQUAL "ASPELL")
	set(SPELLCHECKER_BACKEND "HUNSPELL")
	include("hunspellchecker.cmake")
endif (SPELLCHECKER_BACKEND STREQUAL "ASPELL")

include("${CMAKE_SOURCE_DIR}/src/plugins/plugins.cmake")
