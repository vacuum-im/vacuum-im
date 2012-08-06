#Should be included from SDK
TARGET_UTILS             = vacuumutils
unix:!macx:VERSION_UTILS = 1.17.0

#Configurable values
isEmpty(INSTALL_PREFIX) {
  win32:INSTALL_PREFIX   = "c:"
  unix: INSTALL_PREFIX   = /usr/local
  macx: INSTALL_PREFIX   = /Applications
}

isEmpty(INSTALL_APP_DIR) {
  win32:INSTALL_APP_DIR  = vacuum
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
  unix: INSTALL_RES_DIR  = share
  macx: INSTALL_RES_DIR  = Resources
}

#Path to Vacuum sources direcroty (src)
isEmpty(VACUUM_SRC_PATH) {
  win32:VACUUM_SRC_PATH  = $$INSTALL_PREFIX/$$INSTALL_APP_DIR/src
  unix:VACUUM_SRC_PATH   = $$INSTALL_PREFIX/include/$$INSTALL_APP_DIR
}

#Path to Vacuum builded libraries direcroty (libs)
isEmpty(VACUUM_LIB_PATH) {
  win32:VACUUM_LIB_PATH  = $$INSTALL_PREFIX/$$INSTALL_APP_DIR/$$INSTALL_LIB_DIR
  unix:VACUUM_LIB_PATH   = $$INSTALL_PREFIX/$$INSTALL_LIB_DIR
  macx:VACUUM_LIB_PATH   = $$INSTALL_PREFIX/$$INSTALL_APP_DIR/Contents/$$INSTALL_LIB_DIR 
}

#Debug Features
#include(debug.inc)
