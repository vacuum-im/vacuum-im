include(../make/config.inc)

TEMPLATE            = lib
CONFIG             += plugin
QT                 += widgets network xml
LIBS               += -L../../libs
LIBS               += -l$$VACUUM_UTILS_NAME
DEPENDPATH         += ../..
INCLUDEPATH        += ../..
win32 {
  DLLDESTDIR        = ..\\..\\..\\plugins
  QMAKE_DISTCLEAN  += $${DLLDESTDIR}\\$${TARGET}.dll
}
unix:DESTDIR        = ../../../plugins

#Install
target.path         = $$INSTALL_PLUGINS
INSTALLS           += target

#Translation
TRANS_BUILD_ROOT   = $${OUT_PWD}/../../..
TRANS_SOURCE_ROOT   = ../..
include(../translations/languages.inc)

#MacOS Install
macx {
  UTILS_LIB_NAME   = lib$${VACUUM_UTILS_NAME}.$${VACUUM_UTILS_ABI}.dylib
  name_tool.path    = $$INSTALL_PLUGINS
  name_tool.extra   = install_name_tool -change $$UTILS_LIB_NAME @executable_path/../Frameworks/$$UTILS_LIB_NAME $(INSTALL_ROOT)$$INSTALL_PLUGINS/lib$${TARGET}.dylib
  INSTALLS         += name_tool
}
