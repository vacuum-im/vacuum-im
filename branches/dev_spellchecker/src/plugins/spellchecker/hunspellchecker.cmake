add_definitions(-DHAVE_HUNSPELL) 
add_definitions(-DHUNSPELL_STATIC) 

set(SOURCES ${SOURCES} "hunspellchecker.cpp") 
set(HEADERS ${HEADERS} "hunspellchecker.h")

if (IS_ENABLED)
	if (SYSTEM_HUNSPELL_FOUND)
		target_link_libraries(spellchecker "${SYSTEM_HUNSPELL_FOUND}")
	else (SYSTEM_HUNSPELL_FOUND)
		target_link_libraries(spellchecker hunspell)
	endif (SYSTEM_HUNSPELL_FOUND)
endif (IS_ENABLED) 
