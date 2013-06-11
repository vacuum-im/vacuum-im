HEADERS += screenstate.h \
		   mactools.h

unix:!macx {
	HEADERS += x11tools.h
	SOURCES += x11tools.cpp \
			   screenstate_x11.cpp
}
win32 {
	SOURCES += screenstate_windows.cpp
}