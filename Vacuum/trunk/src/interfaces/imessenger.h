#ifndef IMESSENGER_H
#define IMESSENGER_H

#include <QMainWindow>
#include <QToolBar>
#include <QButtonGroup>
#include <QComboBox>

#define MESSENGER_UUID "{153A4638-B468-496f-B57C-9F30CEDFCC2E}"

class IMessageWindow :
  virtual public QMainWindow
{
public:
  enum ButtonId {
    SendButton,
    ReplyButton,
    QuoteButton,
    ChatButton,
    NextButton,
    CloseButton
  };
public:
  virtual QToolBar *toolBar() const =0;
  virtual QButtonGroup *buttonGroup() const =0;
};

class IMessenger
{
public:
  virtual QObject *instance() = 0;
};

Q_DECLARE_INTERFACE(IMessageWindow,"Vacuum.Plugin.IMessageWindow/1.0")
Q_DECLARE_INTERFACE(IMessenger,"Vacuum.Plugin.IMessenger/1.0")

#endif //IMESSENGER_H