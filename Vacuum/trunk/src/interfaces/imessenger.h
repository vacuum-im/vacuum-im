#ifndef IMESSENGER_H
#define IMESSENGER_H

#include <QMenuBar>
#include <QToolBar>
#include <QButtonGroup>
#include <QTextEdit>
#include "../../utils/jid.h"
#include "../../utils/message.h"

#define MESSENGER_UUID "{153A4638-B468-496f-B57C-9F30CEDFCC2E}"

class IMessageWindow
{
public:
  enum WindowType
  {
    Normal,
    Chat,
    GroupChat,
    Headline
  };
  enum WindowKind 
  {
    ReadOnly,
    WriteOnly,
    ReadWrite
  };
public:
  virtual QObject *instance() = 0;
  virtual WindowType type() const =0;
  virtual WindowKind kind() const =0;
  virtual const Jid &streamJid() const =0;
  virtual void setStreamJid(const Jid &AStreamJid) =0;
  virtual const Jid &contactJid() const =0;
  virtual void setContactJid(const Jid &AContactJid) =0;
};


class IMessenger
{
public:
  virtual QObject *instance() = 0;
};

Q_DECLARE_INTERFACE(IMessageWindow,"Vacuum.Plugin.IMessageWindow/1.0")
Q_DECLARE_INTERFACE(IMessenger,"Vacuum.Plugin.IMessenger/1.0")

#endif //IMESSENGER_H