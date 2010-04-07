/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** Commercial Usage
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain
** additional rights. These rights are described in the Nokia Qt LGPL
** Exception version 1.0, included in the file LGPL_EXCEPTION.txt in this
** package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://www.qtsoftware.com/contact.
**
****************************************************************************/

#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include <QList>
#include <QByteArray>
#include "utilsexport.h"

class UTILS_EXPORT RingBuffer
{
public:
  RingBuffer(int growth = 4096, int maxSize = -1);
  bool isEmpty() const;
  int size() const;
  int maximumSize() const;
  void clear();
  void truncate(int pos);
  void chop(int bytes);
  int skip(int length);
  int write(QByteArray data);
  int write(const char *data, int maxLength);
  QByteArray read(int maxLength);
  int read(char *data, int maxLength);
  bool canReadLine() const;
  int readLine(char *data, int maxLength);
private:
  void free(int bytes);
  char *reserve(int bytes);
  int indexOf(char c) const;
  int nextDataBlockSize() const;
  const char *readPointer() const;
  QByteArray peek(int maxLength) const;
private:
  int head, tail;
  int tailBuffer;
  int basicBlockSize;
  int bufferSize;
  int maxBufferSize;
  QList<QByteArray> buffers;
};

#endif // RINGBUFFER_H
