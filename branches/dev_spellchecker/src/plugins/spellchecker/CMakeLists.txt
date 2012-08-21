project(spellchecker)

set(PLUGIN_NAME "spellchecker")
set(PLUGIN_DISPLAY_NAME "Spellchecker")
set(PLUGIN_DEPENDENCIES messagewidgets) # used only in CPack 

include("spellchecker.cmake")
include("${CMAKE_SOURCE_DIR}/plugins.cmake")

if (USE_ASPELL)
	include("aspellchecker.cmake")
elseif (USE_ENCHANT)
	include("enchantchecker.cmake")
elseif (USE_MACSPELL)
	include("macspellchecker.cmake")
else (USE_ASPELL)
	include("hunspellchecker.cmake")
endif (USE_ASPELL)

