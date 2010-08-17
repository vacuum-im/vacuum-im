set(LOCALIZED_LANGS
	"ru_RU"
	"pl_PL"
#	"uk_UA"
)

macro(add_translations outvar tsname)
	foreach(LANG ${LOCALIZED_LANGS})
		file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/translations/${LANG}")
		set(TS "${CMAKE_SOURCE_DIR}/src/translations/${LANG}/${tsname}.ts")
		set(QM "${CMAKE_BINARY_DIR}/translations/${LANG}/${tsname}.qm")
		add_custom_command(OUTPUT ${QM}
				COMMAND "${QT_LRELEASE_EXECUTABLE}" -silent -compress "${TS}" -qm "${QM}"
				DEPENDS ${TS})
		set(QMS ${QMS} "${QM}")
	endforeach(LANG)
	set(${outvar} "${QMS}")
endmacro(add_translations)
