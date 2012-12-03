/****************************************************************************
*                                                                           *
*   X11tools                                                                *
*   Copyright (C) 2008-2012  Piotr Dąbrowski ultr@ultr.pl                   *
*   Copyright (C) 2011       Przemysław Rudy prudy1@o2.pl                   *
*                                                                           *
*   This program is free software: you can redistribute it and/or modify    *
*   it under the terms of the GNU General Public License as published by    *
*   the Free Software Foundation, either version 2 of the License, or       *
*   (at your option) any later version.                                     *
*                                                                           *
*   This program is distributed in the hope that it will be useful,         *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of          *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
*   GNU General Public License for more details.                            *
*                                                                           *
*   You should have received a copy of the GNU General Public License       *
*   along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
*                                                                           *
****************************************************************************/


// VERSION: 1.24


/*
KNOWN ISSUES:
- Compiz does not return window decoration width and height, however
  it includes it when moving window.
- SetCurrentDesktop() won't work for vertical desktop switching when
  the Compiz's Cube is active (this seems to be the Cube bug)
*/


#ifndef X11TOOLS_H
#define X11TOOLS_H


#include <stdint.h>
#include <string>
#include <utility>
#include <X11/Xlib.h>


#ifdef X11TOOLSDEBUG
	#include <stdio.h>
	#define _x11toolsdebug( ... ) printf( __VA_ARGS__ ); fflush( stdout );
#else
	#define _x11toolsdebug( ... )
#endif


#define  X11_ALLDESKTOPS                 0xFFFFFFFFU  /*uint32_t*/
#define  X11_NODESKTOP                   0xFFFFFFFEU  /*uint32_t*/

#define  X11_SETACTIVEWINDOW_TIMEOUT      (100*1000)  /*usec*/
#define  X11_SETACTIVEWINDOW_CHECKTIME      (2*1000)  /*usec*/

#define  MWM_HINTS_DECORATIONS             (1L << 1)


typedef struct
{
	uint32_t flags;
	uint32_t functions;
	uint32_t decorations;
	 int32_t input_mode;
	uint32_t status;
} MotifWMHints;


bool X11_getCardinalProperty( Display *display, Window window, const char *propertyName, uint32_t *value, unsigned long offset = 0L );
bool X11_getFirstPropertyAtom( Display *display, Window window, const char *propertyName, Atom *value );
bool X11_isPropertyAtomSet( Display *display, Window window, const char *propertyName, const char *atomName );

std::pair<int,int> X11_getResolution( Display *display );
std::pair<int,int> X11_getDesktopSize( Display *display );

std::pair<int,int> X11_getMousePos( Display *display );

bool X11_isPointerGrabbed( Display *display );

bool X11_isFreeDesktopCompatible( Display *display );

uint32_t X11_getDesktopsCount( Display *display, bool forceFreeDesktop = false );
uint32_t X11_getCurrentDesktop( Display *display, bool forceFreeDesktop = false );
void X11_setCurrentDesktop( Display *display, uint32_t desktop, bool forceFreeDesktop = false );
uint32_t X11_getDesktopOfWindow( Display *display, Window window, bool forceFreeDesktop = false, bool windowareadecides = true );
void X11_moveWindowToDesktop( Display *display, Window window, uint32_t desktop, bool forceFreeDesktop = false, bool position = false, int x = 0, int y = 0 );
bool X11_isWindowOnDesktop( Display *display, Window window, uint32_t desktop, bool forceFreeDesktop = false );
bool X11_isWholeWindowOnOneDesktop( Display *display, Window window );
bool X11_isWindowCovered( Display *display, Window window );
bool X11_isWindowShaded( Display *display, Window window );
void X11_shadeWindow( Display *display, Window window, bool shade );

std::pair<int,int> X11_getWindowPos( Display *display, Window window );
std::pair<int,int> X11_getWindowSize( Display *display, Window window );
std::pair<int,int> X11_getWindowFramelessSize( Display *display, Window window );
void X11_moveWindow( Display *display, Window window, int x, int y );
void X11_centerWindow( Display *display, Window window, uint32_t desktop = X11_NODESKTOP, bool forceFreeDesktop = false );
void X11_resizeWindow( Display *display, Window window, int width, int height );
void X11_setSizeHintsOfWindow( Display *display, Window window, int minwidth, int minheight, int maxwidth, int maxheight );

Window X11_getActiveWindow( Display *display );
void X11_setActiveWindow( Display *display, Window window );
void X11_setActiveWindowCheck( Display *display, Window window, bool forceFreeDesktop = false );
Window X11_getTopMostWindow( Display *display );
Window X11_getLatestCreatedWindow( Display *display );

Window X11_getWindowUnderCursor( Display *display, int *rootx = NULL, int *rooty = NULL, int *windowx = NULL, int *windowy = NULL );
Window X11_getInnerMostWindowUnderCursor( Display *display, int *rootx = NULL, int *rooty = NULL, int *windowx = NULL, int *windowy = NULL );

std::string X11_getWindowClass( Display *display, Window window );
std::string X11_getWindowRole( Display *display, Window window );

void X11_windowSendXEvent( Display *display, Window window, const char *type, const char *message, bool set );
void X11_windowChangeProperty( Display *display, Window window, const char *property, const char *value );
void X11_windowSetDecoration( Display *display, Window window, bool set );

bool X11_checkFullScreen( Display *display );

void X11_waitForWindowMapped( Display *display, Window window );

bool X11_isCompositingManagerRunning( Display *display );

void X11_setBlur( Display *display, Window window, bool enable );


#endif
