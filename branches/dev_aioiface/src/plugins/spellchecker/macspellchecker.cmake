add_definitions(-DHAVE_MACSPELL) 

set(ADD_LIBS "-framework AppKit")

set(SOURCES ${SOURCES} "macspellchecker.mm") 
set(HEADERS ${HEADERS} "macspellchecker.h")
