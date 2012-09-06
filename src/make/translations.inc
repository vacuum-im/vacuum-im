#Building Translations
qm_build_all.target = build_qm
for(LANG, TRANS_LANGS) {
  TS_FILE = $${_PRO_FILE_PWD_}/$${TRANS_SOURCE_ROOT}/translations/$${LANG}/$${TRANS_TARGET}.ts
  QM_FILE = $${_PRO_FILE_PWD_}/$${TRANS_SOURCE_ROOT}/translations/$${LANG}/$${TRANS_TARGET}.qm

  LRELEASE = $$[QT_INSTALL_BINS]/lrelease
  win32: LRELEASE = $$replace(LRELEASE, \\\\, \\\\)
  win32: LRELEASE = $$replace(LRELEASE, /, \\\\)

  eval(qm_$${TRANS_TARGET}_build_$${LANG}.target   = $${QM_FILE})
  eval(qm_$${TRANS_TARGET}_build_$${LANG}.depends  = $${TS_FILE})
  eval(qm_$${TRANS_TARGET}_build_$${LANG}.commands = $${LRELEASE} -compress $${TS_FILE} -qm $${QM_FILE})

  qm_build_all.depends += qm_$${TRANS_TARGET}_build_$${LANG}
  QMAKE_EXTRA_TARGETS  += qm_$${TRANS_TARGET}_build_$${LANG}

  win32: QM_FILE = $$replace(QM_FILE, /, \\\\)
  QMAKE_DISTCLEAN += $${QM_FILE}
}
QMAKE_EXTRA_TARGETS  += qm_build_all
POST_TARGETDEPS += $${qm_build_all.target}


#Install Translations
for(LANG, TRANS_LANGS) {
  INST_DIR  = $${INSTALL_TRANSLATIONS}/$${LANG}
  win32: INST_DIR = $$replace(INST_DIR, /, \\\\)
	
  INST_FILE = $${_PRO_FILE_PWD_}/$${TRANS_SOURCE_ROOT}/translations/$${LANG}/$${TRANS_TARGET}.qm
  win32: INST_FILE = $$replace(INST_FILE, /, \\\\)
	
  eval(qm_$${TRANS_TARGET}_inst_$${LANG}.path  = $${INST_DIR})
  eval(qm_$${TRANS_TARGET}_inst_$${LANG}.files = $${INST_FILE})

  #If file is not exits right now, qmake will not install it
  !exists($${INST_FILE}): eval(qm_$${TRANS_TARGET}_inst_$${LANG}.extra = "-$(INSTALL_FILE)" \\\"$${INST_FILE}\\\" \\\"$(INSTALL_ROOT)$${INST_DIR}\\\")

  INSTALLS += qm_$${TRANS_TARGET}_inst_$${LANG}
}
