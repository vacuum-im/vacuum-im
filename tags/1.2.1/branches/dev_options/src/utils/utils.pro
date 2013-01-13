include(../config.inc)

TARGET             = $$TARGET_UTILS
TEMPLATE           = lib
CONFIG            += dll
QT                += xml
DEFINES           += UTILS_DLL
LIBS              += -L../libs
LIBS              += -lidn -lminizip -lzlib
DEPENDPATH        += ..
INCLUDEPATH       += ..
win32 {
  DLLDESTDIR       = ../..
  QMAKE_DISTCLEAN += $${DLLDESTDIR}/$${TARGET}.dll
}
DESTDIR       = ../libs
include(utils.pri)

#Translation
TRANS_SOURCE_ROOT  = ..
include(../translations.inc)

#Install (for Mac OS X - in loader.pro)
!macx:{
  include(../install.inc)
  target.path      = $$INSTALL_LIBS
  INSTALLS         = target
}
