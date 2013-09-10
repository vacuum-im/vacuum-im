#Loader and library names
VACUUM_LOADER_NAME       = vacuum
VACUUM_UTILS_NAME        = vacuumutils

#Library ABI version
unix:VACUUM_UTILS_ABI    = 28

#Build config
CONFIG(debug, debug|release) {
  DEFINES               += DEBUG_MODE
}

#Google Breakpad
!isEmpty(BREAKPAD_DEV_DIR) {
  CONFIG(release, debug|release) {
      QMAKE_CXXFLAGS *= /Zi /DNDEBUG
      QMAKE_CFLAGS *= /Zi /DNDEBUG
      QMAKE_LFLAGS *= /DEBUG /OPT:REF /OPT:ICF
  }
}

#Configurable values
isEmpty(INSTALL_PREFIX) {
  win32:INSTALL_PREFIX   = "c:"
  unix: INSTALL_PREFIX   = /usr/local
  macx: INSTALL_PREFIX   = /Applications
  haiku:INSTALL_PREFIX   = /boot/apps
}

isEmpty(INSTALL_APP_DIR) {
  win32:INSTALL_APP_DIR  = vacuum
  unix: INSTALL_APP_DIR  = vacuum
  macx: INSTALL_APP_DIR  = vacuum.app
  haiku:INSTALL_APP_DIR  = Vacuum
}

isEmpty(INSTALL_LIB_DIR) {
  win32:INSTALL_LIB_DIR  = .
  unix: INSTALL_LIB_DIR  = lib
  macx: INSTALL_LIB_DIR  = Frameworks
  haiku:INSTALL_LIB_DIR  = .
}

isEmpty(INSTALL_RES_DIR) {
  win32:INSTALL_RES_DIR  = .
  unix: INSTALL_RES_DIR  = share
  macx: INSTALL_RES_DIR  = Resources
  haiku:INSTALL_RES_DIR  = .
}

isEmpty(INSTALL_SDK_DIR) {
  win32:INSTALL_SDK_DIR  = sdk
  unix: INSTALL_SDK_DIR  = include
  macx: INSTALL_SDK_DIR  = include
  haiku:INSTALL_SDK_DIR  = sdk
}


#Resources directories
!macx:contains(DEFINES,DEBUG_MODE) {
  DEFINES               += PLUGINS_DIR='\\\"./plugins\\\"'
  DEFINES               += RESOURCES_DIR='\\\"$${PWD}/../../resources\\\"'
  DEFINES               += TRANSLATIONS_DIR='\\\"./src/translations\\\"'
} else:win32 {
  DEFINES               += PLUGINS_DIR='\\\"./$${INSTALL_LIB_DIR}/plugins\\\"'
  DEFINES               += RESOURCES_DIR='\\\"./$${INSTALL_RES_DIR}/resources\\\"'
  DEFINES               += TRANSLATIONS_DIR='\\\"./$${INSTALL_RES_DIR}/translations\\\"'
} else:haiku {
  DEFINES               += PLUGINS_DIR='\\\"./$${INSTALL_LIB_DIR}/plugins\\\"'
  DEFINES               += RESOURCES_DIR='\\\"./$${INSTALL_RES_DIR}/resources\\\"'
  DEFINES               += TRANSLATIONS_DIR='\\\"./$${INSTALL_RES_DIR}/translations\\\"'
} else:macx {
  DEFINES               += PLUGINS_DIR='\\\"../PlugIns\\\"'
  DEFINES               += RESOURCES_DIR='\\\"../$${INSTALL_RES_DIR}\\\"'
  DEFINES               += TRANSLATIONS_DIR='\\\"../$${INSTALL_RES_DIR}/translations\\\"'
} else:unix {
  DEFINES               += PLUGINS_DIR='\\\"../$${INSTALL_LIB_DIR}/$${INSTALL_APP_DIR}/plugins\\\"'
  DEFINES               += RESOURCES_DIR='\\\"../$${INSTALL_RES_DIR}/$${INSTALL_APP_DIR}/resources\\\"'
  DEFINES               += TRANSLATIONS_DIR='\\\"../$${INSTALL_RES_DIR}/$${INSTALL_APP_DIR}/translations\\\"'
}


#Install directories
win32 {
  INSTALL_BINS           = $$INSTALL_PREFIX/$$INSTALL_APP_DIR
  INSTALL_LIBS           = $$INSTALL_PREFIX/$$INSTALL_APP_DIR/$$INSTALL_LIB_DIR
  INSTALL_INCLUDES       = $$INSTALL_PREFIX/$$INSTALL_APP_DIR/$$INSTALL_SDK_DIR
  INSTALL_PLUGINS        = $$INSTALL_PREFIX/$$INSTALL_APP_DIR/$$INSTALL_LIB_DIR/plugins
  INSTALL_RESOURCES      = $$INSTALL_PREFIX/$$INSTALL_APP_DIR/$$INSTALL_RES_DIR/resources
  INSTALL_DOCUMENTS      = $$INSTALL_PREFIX/$$INSTALL_APP_DIR/$$INSTALL_RES_DIR
  INSTALL_TRANSLATIONS   = $$INSTALL_PREFIX/$$INSTALL_APP_DIR/$$INSTALL_RES_DIR/translations
} else:haiku {
  INSTALL_BINS           = $$INSTALL_PREFIX/$$INSTALL_APP_DIR
  INSTALL_LIBS           = $$INSTALL_PREFIX/$$INSTALL_APP_DIR/$$INSTALL_LIB_DIR
  INSTALL_INCLUDES       = $$INSTALL_PREFIX/$$INSTALL_APP_DIR/$$INSTALL_SDK_DIR
  INSTALL_PLUGINS        = $$INSTALL_PREFIX/$$INSTALL_APP_DIR/$$INSTALL_LIB_DIR/plugins
  INSTALL_RESOURCES      = $$INSTALL_PREFIX/$$INSTALL_APP_DIR/$$INSTALL_RES_DIR/resources
  INSTALL_DOCUMENTS      = $$INSTALL_PREFIX/$$INSTALL_APP_DIR/$$INSTALL_RES_DIR/doc
  INSTALL_TRANSLATIONS   = $$INSTALL_PREFIX/$$INSTALL_APP_DIR/$$INSTALL_RES_DIR/translations
} else:macx {
  INSTALL_BINS           = $$INSTALL_PREFIX
  INSTALL_LIBS           = $$INSTALL_PREFIX/$$INSTALL_APP_DIR/Contents/$$INSTALL_LIB_DIR
  INSTALL_INCLUDES       = $$INSTALL_PREFIX/$$INSTALL_APP_DIR/Contents/$$INSTALL_RES_DIR/$$INSTALL_SDK_DIR
  INSTALL_PLUGINS        = $$INSTALL_PREFIX/$$INSTALL_APP_DIR/Contents/PlugIns
  INSTALL_RESOURCES      = $$INSTALL_PREFIX/$$INSTALL_APP_DIR/Contents/$$INSTALL_RES_DIR
  INSTALL_DOCUMENTS      = $$INSTALL_PREFIX/$$INSTALL_APP_DIR/Contents/$$INSTALL_RES_DIR
  INSTALL_TRANSLATIONS   = $$INSTALL_PREFIX/$$INSTALL_APP_DIR/Contents/$$INSTALL_RES_DIR/translations
} else:unix {
  INSTALL_BINS           = $$INSTALL_PREFIX/bin
  INSTALL_LIBS           = $$INSTALL_PREFIX/$$INSTALL_LIB_DIR
  INSTALL_INCLUDES       = $$INSTALL_PREFIX/$$INSTALL_SDK_DIR/$$INSTALL_APP_DIR
  INSTALL_PLUGINS        = $$INSTALL_PREFIX/$$INSTALL_LIB_DIR/$$INSTALL_APP_DIR/plugins
  INSTALL_RESOURCES      = $$INSTALL_PREFIX/$$INSTALL_RES_DIR/$$INSTALL_APP_DIR/resources
  INSTALL_DOCUMENTS      = $$INSTALL_PREFIX/$$INSTALL_RES_DIR/doc/$$INSTALL_APP_DIR
  INSTALL_TRANSLATIONS   = $$INSTALL_PREFIX/$$INSTALL_RES_DIR/$$INSTALL_APP_DIR/translations
}
