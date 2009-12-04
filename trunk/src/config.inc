macx:CONFIG             += x86

isEmpty(INSTALL_PREFIX) {
  win32:INSTALL_PREFIX   = "c:"
  unix:INSTALL_PREFIX    = /usr
  macx:INSTALL_PREFIX    = /Applications
}

isEmpty(INSTALL_DIR) {
  win32:INSTALL_DIR      = vacuum
  unix:INSTALL_DIR       = vacuum
  macx:INSTALL_DIR       = vacuum.app
}
