HEADERS += screenmodechecker.h \

unix:!macx {
	HEADERS += x11tools.h
	SOURCES += x11tools.cpp \
			   screenmode_x11.cpp
}
win32 {
	SOURCES += screenmode_windows.cpp
}
