add_definitions(-DHAVE_ENCHANT) 

if (UNIX)
	FIND_PACKAGE(PkgConfig)
	pkg_check_modules(SYSTEM_ENCHANT enchant>=1.6.0)

	if (SYSTEM_ENCHANT_FOUND)
		set(ADD_LIBS ${SYSTEM_ENCHANT_LIBRARIES})
		message(STATUS "Spellchecker backend: system enchant")
	else (SYSTEM_ENCHANT_FOUND)
		message(FATAL_ERROR "Enchant not found, check enchant installation or use -DSPELLCHECKER_BACKEND=HUNSPELL instead")
		return()
	endif (SYSTEM_ENCHANT_FOUND)
endif (UNIX)

set(SOURCES ${SOURCES} "enchantchecker.cpp")
set(HEADERS ${HEADERS} "enchantchecker.h")

