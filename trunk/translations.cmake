set(LOCALIZED_LANGS
	"ru_RU"
	"pl_PL"
#	"uk_UA"
)

set(LUPDATE_OPTS -no-obsolete -locations none -source-language en)
set(LRELEASE_OPTS -silent -compress)

macro(add_translations outvar tsname)
	set(TS_SRCS "")
	foreach(SRC ${ARGN})
		set(TS_SRCS ${TS_SRCS} "${SRC}")
	endforeach(SRC)
	add_custom_target(updatets_${tsname})
	add_dependencies(updatets updatets_${tsname})
	foreach(LANG ${LOCALIZED_LANGS})
		set(TS "${CMAKE_SOURCE_DIR}/src/translations/${LANG}/${tsname}.ts")
		set(QM "${CMAKE_BINARY_DIR}/translations/${LANG}/${tsname}.qm")
		# Update *.ts
		add_custom_command(TARGET updatets_${tsname} POST_BUILD
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
	endforeach(LANG)
	set(${outvar} "${QMS}")
endmacro(add_translations)
