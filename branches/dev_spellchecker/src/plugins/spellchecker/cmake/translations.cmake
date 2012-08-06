add_custom_target(updatets)

macro(set_enabled_langs langs)
	set(USED_LANGS "")
	if ("${langs}" STREQUAL "")
		set(USED_LANGS ${LOCALIZED_LANGS})
	else ("${langs}" STREQUAL "")
		foreach(lang ${langs})
			list(FIND LOCALIZED_LANGS ${lang} i)
			if (NOT (${i} EQUAL -1))
				list(APPEND USED_LANGS "${lang}")
			endif (NOT (${i} EQUAL -1))
		endforeach(lang)
	endif ("${langs}" STREQUAL "")
endmacro(set_enabled_langs)

macro(add_lang_options)
	foreach(LANG ${LOCALIZED_LANGS})
		set(LANG_${LANG} ON CACHE BOOL "Build *.qm files for language ${LANG}")
	endforeach(LANG)
endmacro(add_lang_options)

macro(process_lang_options)
	set(USED_LANGS "")
	foreach(LANG ${LOCALIZED_LANGS})
		if (${LANG_${LANG}})
			list(APPEND USED_LANGS "${LANG}")
		endif (${LANG_${LANG}})
	endforeach(LANG)
endmacro(process_lang_options)

macro(lang_display_name outvar langname)
	set(${outvar} "${langname}")
	if (${langname} STREQUAL "ru")
		set(${outvar} "Russian")
	elseif (${langname} STREQUAL "pl")
		set(${outvar} "Polish")
	elseif (${langname} STREQUAL "uk")
		set(${outvar} "Ukrainian")
	elseif (${langname} STREQUAL "de")
		set(${outvar} "German")
	endif (${langname} STREQUAL "ru")
endmacro(lang_display_name)

set(LUPDATE_OPTS -no-obsolete -locations none -source-language en)
set(LRELEASE_OPTS -silent -compress)

macro(add_translations outvar tsname)
	set(TS_SRCS "")
	foreach(SRC ${ARGN})
		set(TS_SRCS ${TS_SRCS} "${SRC}")
	endforeach(SRC)
	foreach(LANG ${USED_LANGS})
		set(TS "${CMAKE_SOURCE_DIR}/translations/${LANG}/${tsname}.ts")
		set(QM "${CMAKE_BINARY_DIR}/translations/${LANG}/${tsname}.qm")
		# Update *.ts
		add_custom_command(TARGET updatets POST_BUILD
				COMMAND "${QT_LUPDATE_EXECUTABLE}" ${LUPDATE_OPTS} ${TS_SRCS} -ts "${TS}" 
				DEPENDS ${TS_SRCS}
				WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
				COMMENT "Updating ${TS}")
		# Generate *.qm
		file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/translations/${LANG}")
		add_custom_command(OUTPUT ${QM}
				COMMAND "${QT_LRELEASE_EXECUTABLE}" ${LRELEASE_OPTS} "${TS}" -qm "${QM}"
				DEPENDS ${TS})
		set(QMS ${QMS} "${QM}")
		# Install *.qm
		install(FILES "${QM}" DESTINATION "${INSTALL_TRANSLATIONS}/${LANG}")
	endforeach(LANG)
	set(${outvar} "${QMS}")
endmacro(add_translations)

set(LANGS "" CACHE STRING "List of languages to build localization for (this variable overrides any of LANG_*)")
set(USED_LANGS "" CACHE INTERNAL "List of languages actually used for generating targets")
add_lang_options()

if ("${LANGS}" STREQUAL "")
	process_lang_options()
else ("${LANGS}" STREQUAL "")
	set_enabled_langs("${LANGS}")
endif ("${LANGS}" STREQUAL "")
