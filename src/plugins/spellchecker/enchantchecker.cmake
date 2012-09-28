add_definitions(-DHAVE_ENCHANT) 

if (UNIX)
	set(ADD_LIBS ${SYSTEM_ENCHANT_LIBRARIES})
	message(STATUS "Spellchecker backend: system enchant")
endif (UNIX)

set(SOURCES ${SOURCES} "enchantchecker.cpp")
set(HEADERS ${HEADERS} "enchantchecker.h")

