TRANS_LANGS  = ru_RU pl_PL de_DE uk_UA

FIRST_TARGET = $$member(TARGET,0)
TRANSLATIONS = \
	$${TRANS_SOURCE_ROOT}/translations/ru_RU/$${FIRST_TARGET}.ts \
	$${TRANS_SOURCE_ROOT}/translations/pl_PL/$${FIRST_TARGET}.ts \
	$${TRANS_SOURCE_ROOT}/translations/de_DE/$${FIRST_TARGET}.ts \
	$${TRANS_SOURCE_ROOT}/translations/uk_UA/$${FIRST_TARGET}.ts 


#Building Translations
qm_all.target = build_qm
for(LANG, TRANS_LANGS) {
	TS_FILE = $${TRANS_SOURCE_ROOT}/translations/$${LANG}/$${FIRST_TARGET}.ts
	QM_FILE = $${TRANS_SOURCE_ROOT}/../translations/$${LANG}/$${FIRST_TARGET}.qm

	QM_DIR = $${TRANS_SOURCE_ROOT}/../translations/$${LANG}
	win32: QM_DIR = $$replace(QM_DIR, "/", "\\")
	
	LRELEASE = $$[QT_INSTALL_BINS]/lrelease
	win32: LRELEASE = $$replace(LRELEASE, "/", "\\")

	!win32:IF_DELIM = ||

	eval(qm_$${LANG}.target   = $${QM_FILE})
	eval(qm_$${LANG}.depends  = $${TS_FILE})
	eval(qm_$${LANG}.commands = ("$(CHK_DIR_EXISTS)" $${QM_DIR} $${IF_DELIM} "$(MKDIR)" $${QM_DIR}) && ($${LRELEASE} -compress $${TS_FILE} -qm $${QM_FILE}))

	qm_all.depends += qm_$${LANG}
	QMAKE_EXTRA_TARGETS  += qm_$${LANG}
}
QMAKE_EXTRA_TARGETS  += qm_all
POST_TARGETDEPS += $${qm_all.target}
