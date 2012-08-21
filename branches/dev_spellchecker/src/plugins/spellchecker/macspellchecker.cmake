add_definitions(-DHAVE_MACSPELL) 

set(SOURCES ${SOURCES} "macspellchecker.mm") 
set(HEADERS ${HEADERS} "macspellchecker.h")
set(MOC_HEADERS ${MOC_HEADERS} "macspellchecker.h") 

if (IS_ENABLED)
	target_link_libraries(spellchecker "-framework AppKit")
endif (IS_ENABLED)
