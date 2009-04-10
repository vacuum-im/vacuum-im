#ifndef IMESSAGESTYLES_H
#define IMESSAGESTYLES_H

#include <QColor>
#include <QString>
#include <QVariant>
#include <QDateTime>
#include <QStringList>

#define MESSAGESTYLES_UUID  "{e3ab1bc7-35a6-431a-9b91-c778451b1eb1}"

class IMessageStyle
{
public:
  struct StyleOptions {
    QString variant;
    QString chatName;
    QString accountName;
    QString selfName;
    QString contactName;
    QString selfAvatar;
    QString contactAvatar;
    QDateTime startTime;
    enum {
      BackgroundNormal,
      BackgroundCenter,
      BackgroundTitle,
      BackgroundTitleCenter,
      BackgroundScale
    } backgroundType;
    QColor backgroundColor;
    QString backgroundImage;
  };
  struct ContentOptions {
    bool isStatus;
    bool isAction;
    bool isArchive;
    bool isAlignLTR;
    bool isSameSender;
    bool isDirectionIn;
    bool willAppendMoreContent;
    bool replaceLastContent;
    QDateTime sendTime;
    QString senderName;
    QString senderAvatar;
    QString senderStatusIcon;
    QString senderColor;
    QString statusHint;
    QString backgroundColor;
    QStringList messageClasses;
  };
public:
  virtual QObject *instance() = 0;
};

class IMessageStyles 
{
public:
  virtual QObject *instance() = 0;
};

Q_DECLARE_INTERFACE(IMessageStyle,"Vacuum.Plugin.IMessageStyle/1.0")
Q_DECLARE_INTERFACE(IMessageStyles,"Vacuum.Plugin.IMessageStyles/1.0")

#endif
