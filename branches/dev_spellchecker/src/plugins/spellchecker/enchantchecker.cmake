add_definitions(-DHAVE_ENCHANT) 

set(SOURCES ${SOURCES} "enchantchecker.cpp") 
set(HEADERS ${HEADERS} "enchantchecker.h")
set(MOC_HEADERS ${MOC_HEADERS} "enchantchecker.h") 

if (IS_ENABLED)
	target_link_libraries(spellchecker enchant)
endif (IS_ENABLED)
