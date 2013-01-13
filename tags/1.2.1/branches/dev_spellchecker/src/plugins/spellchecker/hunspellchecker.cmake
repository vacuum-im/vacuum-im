add_definitions(-DHAVE_HUNSPELL) 
add_definitions(-DHUNSPELL_STATIC) 

if (SYSTEM_HUNSPELL_FOUND)
	set(ADD_LIBS ${SYSTEM_HUNSPELL_FOUND})
else (SYSTEM_HUNSPELL_FOUND)
	set(ADD_LIBS hunspell)
endif (SYSTEM_HUNSPELL_FOUND)

set(SOURCES ${SOURCES} "hunspellchecker.cpp")
set(HEADERS ${HEADERS} "hunspellchecker.h")
