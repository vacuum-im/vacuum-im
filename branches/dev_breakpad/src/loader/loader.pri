FORMS   = setuppluginsdialog.ui \
          aboutbox.ui

HEADERS = pluginmanager.h \
          setuppluginsdialog.h \
          aboutbox.h

SOURCES = main.cpp \
          pluginmanager.cpp \
          setuppluginsdialog.cpp \
          aboutbox.cpp

#Google Breakpad
!isEmpty(BREAKPAD_DEV_DIR) {
  HEADERS += crashhandler.h
  SOURCES += crashhandler.cpp
}