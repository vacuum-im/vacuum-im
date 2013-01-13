add_definitions(-DHAVE_MACSPELL) 

set(ADD_LIBS "-framework AppKit")

message(STATUS "Spellchecker backend: macspell")

set(SOURCES ${SOURCES} "macspellchecker.mm") 
set(HEADERS ${HEADERS} "macspellchecker.h")
