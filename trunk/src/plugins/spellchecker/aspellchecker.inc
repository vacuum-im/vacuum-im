DEFINES       += HAVE_ASPELL
win32 {
  LIBS        += -laspell-15
  LIBS        += -L$${ASPELL_DEV_PATH}/lib
  INCLUDEPATH += $${ASPELL_DEV_PATH}/include
} else {
  LIBS        += -laspell
}
HEADERS       += aspellchecker.h
SOURCES       += aspellchecker.cpp
