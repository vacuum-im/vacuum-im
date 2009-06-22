TARGET = clientinfo
include(clientinfo.pri)
include(../plugins.inc)

win32:system(subwcrev ../../../. svninfo.tmpl svninfo.h > nul) {
  DEFINES += SVNINFO
}
