/****************************************************************************
 **
 ** Copyright (C) Qxt Foundation. Some rights reserved.
 **
 ** This file is part of the QxtGui module of the Qxt library.
 **
 ** This library is free software; you can redistribute it and/or modify it
 ** under the terms of the Common Public License, version 1.0, as published
 ** by IBM, and/or under the terms of the GNU Lesser General Public License,
 ** version 2.1, as published by the Free Software Foundation.
 **
 ** This file is provided "AS IS", without WARRANTIES OR CONDITIONS OF ANY
 ** KIND, EITHER EXPRESS OR IMPLIED INCLUDING, WITHOUT LIMITATION, ANY
 ** WARRANTIES OR CONDITIONS OF TITLE, NON-INFRINGEMENT, MERCHANTABILITY OR
 ** FITNESS FOR A PARTICULAR PURPOSE.
 **
 ** You should have received a copy of the CPL and the LGPL along with this
 ** file. See the LICENSE file and the cpl1.0.txt/lgpl-2.1.txt files
 ** included with the source distribution for more information.
 ** If you did not receive a copy of the licenses, contact the Qxt Foundation.
 **
 ** <http://libqxt.org>  <foundation@libqxt.org>
 **
 ****************************************************************************/
#include "qxtglobalshortcut_p.h"
#include <InterfaceDefs.h>

#include <QWidget>
#include <QCoreApplication>

bool QxtGlobalShortcutPrivate::eventFilter(void* message)
{
    BMessage *msg = static_cast<BMessage*>(message);
 //   if (msg->message == WM_HOTKEY)
   // {
     //   const quint32 keycode = HIWORD(msg->lParam);
       // const quint32 modifiers = LOWORD(msg->lParam);
       // activateShortcut(keycode, modifiers);
    //}
    return false;
}
quint32 QxtGlobalShortcutPrivate::nativeModifiers(Qt::KeyboardModifiers modifiers)
{
    // ShiftMask, LockMask, ControlMask, Mod1Mask, Mod2Mask, Mod3Mask, Mod4Mask, and Mod5Mask
/*    quint32 native = 0;
    if (modifiers & Qt::ShiftModifier)
        native |= ShiftMask;
    if (modifiers & Qt::ControlModifier)
        native |= ControlMask;
    if (modifiers & Qt::AltModifier)
        native |= Mod1Mask;
    if (modifiers & Qt::MetaModifier)
        native |= Mod4Mask;
*/
    // TODO: resolve these?
    //if (modifiers & Qt::MetaModifier)
    //if (modifiers & Qt::KeypadModifier)
    //if (modifiers & Qt::GroupSwitchModifier)
    //return native;
}

quint32 QxtGlobalShortcutPrivate::nativeKeycode(Qt::Key key)
{
    switch (key)
    {
    case Qt::Key_Escape:
        return B_ESCAPE;
    case Qt::Key_Tab:
    case Qt::Key_Backtab:
        return B_TAB;
    case Qt::Key_Backspace:
        return B_BACKSPACE;
    case Qt::Key_Return:
        return B_RETURN;
    case Qt::Key_Enter:
        return B_ENTER;
    case Qt::Key_Insert:
        return B_INSERT;
    case Qt::Key_Delete:
        return B_DELETE;
    case Qt::Key_Pause:
        return B_PAUSE_KEY;
    case Qt::Key_Print:
        return B_PRINT_KEY;
//  case Qt::Key_Clear:
//      return B_CLEAR;
    case Qt::Key_Home:
        return B_HOME;
    case Qt::Key_End:
        return B_END;
    case Qt::Key_Left:
        return B_LEFT_ARROW;
    case Qt::Key_Up:
        return B_UP_ARROW;
    case Qt::Key_Right:
        return B_RIGHT_ARROW;
    case Qt::Key_Down:
        return B_DOWN_ARROW;
    case Qt::Key_PageUp:
        return B_PAGE_UP;
    case Qt::Key_PageDown:
        return B_PAGE_DOWN;
    case Qt::Key_F1:
        return B_F1_KEY;
    case Qt::Key_F2:
        return B_F2_KEY;
    case Qt::Key_F3:
        return B_F3_KEY;
    case Qt::Key_F4:
        return B_F4_KEY;
    case Qt::Key_F5:
        return B_F5_KEY;
    case Qt::Key_F6:
        return B_F6_KEY;
    case Qt::Key_F7:
        return B_F7_KEY;
    case Qt::Key_F8:
        return B_F8_KEY;
    case Qt::Key_F9:
        return B_F9_KEY;
    case Qt::Key_F10:
        return B_F10_KEY;
    case Qt::Key_F11:
        return B_F11_KEY;
    case Qt::Key_F12:
        return B_F12_KEY;
    case Qt::Key_Space:
        return B_SPACE;
/*    case Qt::Key_Asterisk:
        return B_MULTIPLY;
    case Qt::Key_Plus:
        return B_ADD;
    case Qt::Key_Comma:
        return B_SEPARATOR;
    case Qt::Key_Minus:
        return B_SUBTRACT;
    case Qt::Key_Slash:
        return B_DIVIDE;
    case Qt::Key_MediaNext:
        return B_MEDIA_NEXT_TRACK;
    case Qt::Key_MediaPrevious:
        return B_MEDIA_PREV_TRACK;
    case Qt::Key_MediaPlay:
        return B_MEDIA_PLAY_PAUSE;
    case Qt::Key_MediaStop:
        return B_MEDIA_STOP;
        // couldn't find those in B_*
        //case Qt::Key_MediaLast:
        //case Qt::Key_MediaRecord:
    case Qt::Key_VolumeDown:
        return B_VOLUME_DOWN;
    case Qt::Key_VolumeUp:
        return B_VOLUME_UP;
    case Qt::Key_VolumeMute:
        return B_VOLUME_MUTE;
*/
        // numbers
    case Qt::Key_0:
    case Qt::Key_1:
    case Qt::Key_2:
    case Qt::Key_3:
    case Qt::Key_4:
    case Qt::Key_5:
    case Qt::Key_6:
    case Qt::Key_7:
    case Qt::Key_8:
    case Qt::Key_9:
        return key;

        // letters
    case Qt::Key_A:
    case Qt::Key_B:
    case Qt::Key_C:
    case Qt::Key_D:
    case Qt::Key_E:
    case Qt::Key_F:
    case Qt::Key_G:
    case Qt::Key_H:
    case Qt::Key_I:
    case Qt::Key_J:
    case Qt::Key_K:
    case Qt::Key_L:
    case Qt::Key_M:
    case Qt::Key_N:
    case Qt::Key_O:
    case Qt::Key_P:
    case Qt::Key_Q:
    case Qt::Key_R:
    case Qt::Key_S:
    case Qt::Key_T:
    case Qt::Key_U:
    case Qt::Key_V:
    case Qt::Key_W:
    case Qt::Key_X:
    case Qt::Key_Y:
    case Qt::Key_Z:
        return key;

    default:
        return 0;
    }
}

bool QxtGlobalShortcutPrivate::registerShortcut(quint32 nativeKey, quint32 nativeMods)
{
  //  return RegisterHotKey(0, nativeMods ^ nativeKey, nativeMods, nativeKey);
}

bool QxtGlobalShortcutPrivate::unregisterShortcut(quint32 nativeKey, quint32 nativeMods)
{
   // return UnregisterHotKey(0, nativeMods ^ nativeKey);
}
