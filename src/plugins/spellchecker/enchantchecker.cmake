add_definitions(-DHAVE_ENCHANT) 

set(ADD_LIBS enchant)

message(STATUS "Spellchecker backend: enchant")

set(SOURCES ${SOURCES} "enchantchecker.cpp")
set(HEADERS ${HEADERS} "enchantchecker.h")

