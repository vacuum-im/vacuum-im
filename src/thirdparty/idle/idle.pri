HEADERS += idle.h
SOURCES += idle.cpp

unix:!macx {
	SOURCES += idle_x11.cpp
}
win32 {
	SOURCES += idle_win.cpp
}
mac {
	SOURCES += idle_mac.cpp
}
