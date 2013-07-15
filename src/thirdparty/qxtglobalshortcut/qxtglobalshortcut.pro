include(../../make/config.inc)

TARGET         = qxtglobalshortcut
TEMPLATE       = lib
CONFIG        += staticlib warn_off
DESTDIR        = ../../libs
DEFINES       += QXT_STATIC
isEqual(QT_MAJOR_VERSION, 5) {
	QT += gui-private
}
include(qxtglobalshortcut.pri)
