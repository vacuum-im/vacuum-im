TARGET_LOADER            = vacuum

TARGET_UTILS             = vacuumutils
unix:VERSION_UTILS       = 1.12.0

#Configurable values
isEmpty(INSTALL_PREFIX) {
  win32:INSTALL_PREFIX   = "c:"
  haiku:INSTALL_PREFIX   = /boot/apps
  unix: INSTALL_PREFIX   = /usr/local
  macx: INSTALL_PREFIX   = /Applications
}

isEmpty(INSTALL_APP_DIR) {
  win32:INSTALL_APP_DIR  = vacuum
  haiku:INSTALL_APP_DIR  = Vacuum
  unix: INSTALL_APP_DIR  = vacuum
  macx: INSTALL_APP_DIR  = vacuum.app
}

isEmpty(INSTALL_LIB_DIR) {
  win32:INSTALL_LIB_DIR  = .
  unix: INSTALL_LIB_DIR  = lib
  macx: INSTALL_LIB_DIR  = Frameworks
}

isEmpty(INSTALL_RES_DIR) {
  win32:INSTALL_RES_DIR  = .
  haiku:INSTALL_RES_DIR  = .
  unix: INSTALL_RES_DIR  = share
  macx: INSTALL_RES_DIR  = Resources
}


#Preprocessor definations
!macx:CONFIG(debug, debug|release) {
  DEFINES               += PLUGINS_DIR='\\\"./plugins\\\"'
  DEFINES               += RESOURCES_DIR='\\\"./resources\\\"'
  DEFINES               += TRANSLATIONS_DIR='\\\"./src/translations\\\"'
} else:win32 {
  DEFINES               += PLUGINS_DIR='\\\"./$${INSTALL_LIB_DIR}/plugins\\\"'
  DEFINES               += RESOURCES_DIR='\\\"./$${INSTALL_RES_DIR}/resources\\\"'
  DEFINES               += TRANSLATIONS_DIR='\\\"./$${INSTALL_RES_DIR}/translations\\\"'
} else:unix:!macx :!haiku {
  DEFINES               += PLUGINS_DIR='\\\"../$${INSTALL_LIB_DIR}/$${INSTALL_APP_DIR}/plugins\\\"'
  DEFINES               += RESOURCES_DIR='\\\"../$${INSTALL_RES_DIR}/$${INSTALL_APP_DIR}/resources\\\"'
  DEFINES               += TRANSLATIONS_DIR='\\\"../$${INSTALL_RES_DIR}/$${INSTALL_APP_DIR}/translations\\\"'
} else:macx {
  DEFINES               += PLUGINS_DIR='\\\"../PlugIns\\\"'
  DEFINES               += RESOURCES_DIR='\\\"../$${INSTALL_RES_DIR}\\\"'
  DEFINES               += TRANSLATIONS_DIR='\\\"../$${INSTALL_RES_DIR}/translations\\\"'
} else:haiku {
  DEFINES               += PLUGINS_DIR='\\\"./$${INSTALL_RES_DIR}/plugins\\\"'
  DEFINES               += RESOURCES_DIR='\\\"./$${INSTALL_RES_DIR}/resources\\\"'
  DEFINES               += TRANSLATIONS_DIR='\\\"./$${INSTALL_RES_DIR}/translations\\\"'
}
