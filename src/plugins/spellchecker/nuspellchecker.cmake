add_definitions(-DHAVE_NUSPELL)
#add_definitions(-DHUNSPELL_STATIC)

#if (WIN32)
#	add_subdirectory(../../thirdparty/hunspell hunspell)
#	set(ADD_LIBS hunspell)
#	message(STATUS "Spellchecker backend: bundled hunspell")
#endif (WIN32)

if (UNIX)
        FIND_PACKAGE(Nuspell REQUIRED)
        message(STATUS "Spellchecker backend: system nuspell")
endif (UNIX)

set(SOURCES ${SOURCES} "nuspellchecker.cpp")
set(HEADERS ${HEADERS} "nuspellchecker.h")
