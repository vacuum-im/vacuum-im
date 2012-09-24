add_definitions(-DHAVE_ENCHANT) 

if(SYSTEM_ENCHANT_FOUND)
	set(ADD_LIBS enchant)
else(SYSTEM_ENCHANT_FOUND)
	message(FATAL_ERROR "Enchant not found, check enchant installation or use -DSPELLCHECKER_BACKEND=HUNSPELL instead")
endif(SYSTEM_ENCHANT_FOUND)

message(STATUS "Spellchecker backend: enchant")

set(SOURCES ${SOURCES} "enchantchecker.cpp")
set(HEADERS ${HEADERS} "enchantchecker.h")

