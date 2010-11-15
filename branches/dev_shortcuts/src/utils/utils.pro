include(../config.inc)

TARGET             = $$TARGET_UTILS
TEMPLATE           = lib
VERSION            = $$VERSION_UTILS
CONFIG            += dll
QT                += xml
DEFINES           += UTILS_DLL QXT_STATIC
LIBS              += -L../libs
unix:!macx {
  LIBS            += -lidn -lminizip -lz -lqxtglobalshortcut
} else {
  LIBS            += -lidn -lminizip -lzlib -lqxtglobalshortcut
  INCLUDEPATH     += ../thirdparty/zlib
}
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
