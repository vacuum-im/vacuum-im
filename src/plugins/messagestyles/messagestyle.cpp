#include "messagestyle.h"

#include <QtDebug>

#include <QUrl>
#include <QDir>
#include <QFile>
#include <QRegExp>
#include <QWebFrame>
#include <QByteArray>
#include <QStringList>
#include <QDomDocument>
#include <QTextDocument>
#include <QCoreApplication>

#define SHARED_STYLE_PATH                   STORAGE_DIR"/"RSR_STORAGE_MESSAGESTYLES"/"STORAGE_SHARED_DIR
#define STYLE_CONTENTS_PATH                 "Contents"
#define STYLE_RESOURCES_PATH                STYLE_CONTENTS_PATH"/Resources"

#define APPEND_MESSAGE_WITH_SCROLL		      "checkIfScrollToBottomIsNeeded(); appendMessage(\"%1\"); scrollToBottomIfNeeded();"
#define APPEND_NEXT_MESSAGE_WITH_SCROLL	    "checkIfScrollToBottomIsNeeded(); appendNextMessage(\"%1\"); scrollToBottomIfNeeded();"
#define APPEND_MESSAGE					            "appendMessage(\"%1\");"
#define APPEND_NEXT_MESSAGE				          "appendNextMessage(\"%1\");"
#define APPEND_MESSAGE_NO_SCROLL		        "appendMessageNoScroll(\"%1\");"
#define	APPEND_NEXT_MESSAGE_NO_SCROLL	      "appendNextMessageNoScroll(\"%1\");"
#define REPLACE_LAST_MESSAGE			          "replaceLastMessage(\"%1\");"

MessageStyle::MessageStyle(const QString &AStylePath, QObject *AParent) : QObject(AParent)
{
  FInfo = styleInfo(AStylePath);
  FVariants = styleVariants(AStylePath);
  FResourcePath = AStylePath + "/"STYLE_RESOURCES_PATH;
  loadTemplates();
}

MessageStyle::~MessageStyle()
{

}

bool MessageStyle::isValid() const
{
  return !FIn_ContentHTML.isEmpty() && !FStatusHTML.isEmpty();
}

int MessageStyle::version() const
{
  return FInfo.value("MessageViewVersion",0).toInt();
}

QList<QString> MessageStyle::variants() const
{
  return FVariants;
}

QVariant MessageStyle::infoValue(const QString &AKey, const QVariant &ADefValue) const
{
  return FInfo.value(AKey,ADefValue);
}

void MessageStyle::setStyle(QWebView *AView, const IMessageStyle::StyleOptions &AOptions) const
{
  QString html = makeStyleTemplate(AOptions);
  fillStyleKeywords(html,AOptions);
  AView->setHtml(html);
  emit styleSeted(AView,AOptions);
}

void MessageStyle::setVariant(QWebView *AView, const QString &AVariant) const
{
  QString variant = QDir::cleanPath(QString("Variants/%1.css").arg(!FVariants.contains(AVariant) ? FInfo.value("DefaultVariant","../main").toString() : AVariant));
  QString script = QString("setStylesheet(\"%1\",\"%2\");").arg("mainStyle").arg(variant);
  AView->page()->mainFrame()->evaluateJavaScript(script);
  emit variantSeted(AView,AVariant);
}

void MessageStyle::appendContent(QWebView *AView, const QString &AMessage, const IMessageStyle::ContentOptions &AOptions) const
{
  QString html = makeContentTemplate(AOptions);
  fillContentKeywords(html,AOptions);
  html.replace("%message%",AMessage);

  escapeStringForScript(html);
  AView->page()->mainFrame()->evaluateJavaScript(scriptForAppendContent(AOptions).arg(html));

  emit contentAppended(AView,AMessage,AOptions);
}

QString MessageStyle::makeStyleTemplate(const IMessageStyle::StyleOptions &AOptions) const
{
  bool usingCustomTemplate = true;
  QString htmlFileName = FResourcePath+"/Template.html";
  if (!QFile::exists(htmlFileName))
  {
    usingCustomTemplate = false;
    htmlFileName = qApp->applicationDirPath()+"/"SHARED_STYLE_PATH"/"STYLE_RESOURCES_PATH"/Template.html";
  }

  QString html = loadFileData(htmlFileName,QString::null);
  if (!html.isEmpty())
  {
    QString variant = QDir::cleanPath(QString("Variants/%1.css").arg(!FVariants.contains(AOptions.variant) ? FInfo.value("DefaultVariant","../main").toString() : AOptions.variant));

    if (version()<3 && usingCustomTemplate)
    {
      html.replace(html.indexOf("%@"),2,FResourcePath+"/");
      html.replace(html.indexOf("%@"),2,variant);
      html.replace(html.indexOf("%@"),2,loadFileData(FResourcePath+"/Header.html",QString::null));
      html.replace(html.indexOf("%@"),2,loadFileData(FResourcePath+"/Footer.html",QString::null));
    }
    else
    {
      html.replace(html.indexOf("%@"),2,FResourcePath+"/");
      html.replace(html.indexOf("%@"),2,version()<3 ? "" : "@import url( \"main.css\" );");
      html.replace(html.indexOf("%@"),2,variant);
      html.replace(html.indexOf("%@"),2,loadFileData(FResourcePath+"/Header.html",QString::null));
      html.replace(html.indexOf("%@"),2,loadFileData(FResourcePath+"/Footer.html",QString::null));
    }
  }
  return html;
}

QString MessageStyle::makeContentTemplate(const IMessageStyle::ContentOptions &AOptions) const
{
  QString html;
  if (AOptions.isStatus)
  {
    html = FStatusHTML;
  }
  else if (AOptions.isDirectionIn)
  {
    if (AOptions.isAction && !FIn_ActionHTML.isEmpty())
      html = FIn_ActionHTML;
    else if (!AOptions.isArchive)
      html = AOptions.isSameSender ? FIn_NextContentHTML : FIn_ContentHTML;
    else
      html = AOptions.isSameSender ? FIn_NextContextHTML : FIn_ContextHTML;
  }
  else
  {
    if (AOptions.isAction && !FOut_ActionHTML.isEmpty())
      html = FOut_ActionHTML;
    else if (!AOptions.isArchive)
      html = AOptions.isSameSender ? FOut_NextContentHTML : FOut_ContentHTML;
    else
      html = AOptions.isSameSender ? FOut_NextContextHTML : FOut_ContextHTML;
  }
  return html;
}

void MessageStyle::fillStyleKeywords(QString &AHtml, const IMessageStyle::StyleOptions &AOptions) const
{
  AHtml.replace("%chatName%",AOptions.chatName);
  AHtml.replace("%sourceName%", AOptions.accountName);
  AHtml.replace("%destinationName%", AOptions.chatName);
  AHtml.replace("%destinationDisplayName%", AOptions.chatName);
  AHtml.replace("%incomingIconPath%", !AOptions.contactAvatar.isEmpty() ? AOptions.contactAvatar : "incoming_icon.png");
  AHtml.replace("%outgoingIconPath%", !AOptions.selfAvatar.isEmpty() ? AOptions.selfAvatar : "outgoing_icon.png");
  AHtml.replace("%serviceIconImg%", "");
  AHtml.replace("%timeOpened%", Qt::escape(AOptions.startTime.toString()));

  QString background;
  if (!AOptions.backgroundImage.isEmpty())
  {
    if (AOptions.backgroundType == IMessageStyle::StyleOptions::BackgroundNormal)
      background.append("background-image: url('%1'); background-repeat: no-repeat; background-attachment:fixed;");
    else if (AOptions.backgroundType == IMessageStyle::StyleOptions::BackgroundCenter)
      background.append("background-image: url('%1'); background-position: center; background-repeat: no-repeat; background-attachment:fixed;");
    else if (AOptions.backgroundType == IMessageStyle::StyleOptions::BackgroundTitle)
      background.append("background-image: url('%1'); background-repeat: repeat;");
    else if (AOptions.backgroundType == IMessageStyle::StyleOptions::BackgroundTitleCenter)
      background.append("background-image: url('%1'); background-repeat: repeat; background-position: center;");
    else if (AOptions.backgroundType == IMessageStyle::StyleOptions::BackgroundScale)
      background.append("background-image: url('%1'); -webkit-background-size: 100% 100%; background-size: 100% 100%; background-attachment: fixed;");
    background = background.arg(AOptions.backgroundImage);
  }
  if (AOptions.backgroundColor.isValid())
  {
    int r,g,b,a;
    AOptions.backgroundColor.getRgb(&r,&g,&b,&a);
    background.append(QString("background-color: rgba(%1, %2, %3, %4);").arg(r).arg(g).arg(b).arg(qreal(a)/255.0));
  }
  AHtml.replace("==bodyBackground==", background);
}

void MessageStyle::fillContentKeywords(QString &AHtml, const IMessageStyle::ContentOptions &AOptions) const
{
  AHtml.replace("%sender%",AOptions.senderName);
  AHtml.replace("%senderScreenName%",AOptions.senderName);
  AHtml.replace("%senderDisplayName%",AOptions.senderName);
  AHtml.replace("%senderStatusIcon%",AOptions.senderStatusIcon);
  AHtml.replace("%service%","");
  AHtml.replace("%status%",AOptions.statusHint);
  AHtml.replace("%messageClasses%", AOptions.messageClasses.join(" "));
  AHtml.replace("%messageDirection%", AOptions.isAlignLTR ? "ltr" : "rtl" );

  if (!AOptions.senderAvatar.isEmpty())
    AHtml.replace("%userIconPath%",AOptions.senderAvatar);
  else
    AHtml.replace("%userIconPath%",AOptions.isDirectionIn ? "Incoming/buddy_icon.png" : "Outgoing/buddy_icon.png");

  QString sColor = !AOptions.senderColor.isEmpty() ? AOptions.senderColor : "inherit";
  AHtml.replace("%senderColor%",sColor);
  QRegExp scolorRegExp("%senderColor\\{([^}]*)\\}%");
  for (int pos=0; pos!=-1; pos = scolorRegExp.indexIn(AHtml, pos))
    if (!scolorRegExp.cap(0).isEmpty())
      AHtml.replace(pos, scolorRegExp.cap(0).length(), sColor);

  QString bgColor = !AOptions.backgroundColor.isEmpty() ? AOptions.backgroundColor : "inherit";
  QRegExp colorRegExp("%textbackgroundcolor\\{([^}]*)\\}%");
  for (int pos=0; pos!=-1; pos = colorRegExp.indexIn(AHtml, pos))
    if (!colorRegExp.cap(0).isEmpty())
      AHtml.replace(pos, colorRegExp.cap(0).length(), bgColor);

  QString time = Qt::escape(AOptions.sendTime.time().toString());
  AHtml.replace("%time%", time);
  AHtml.replace("%shortTime%", time);
  QRegExp timeRegExp("%time\\{([^}]*)\\}%");
  for (int pos=0; pos!=-1; pos = timeRegExp.indexIn(AHtml, pos))
    if (!timeRegExp.cap(0).isEmpty())
      AHtml.replace(pos, timeRegExp.cap(0).length(), time);
}

void MessageStyle::escapeStringForScript(QString &AText) const
{
  AText.replace("\\","\\\\");
  AText.replace("\"","\\\"");
  AText.replace("\n","");
  AText.replace("\r","<br>");
}

QString MessageStyle::scriptForAppendContent(const IMessageStyle::ContentOptions &AOptions) const
{
  QString script;

  if (version() >= 4) 
  {
    if (AOptions.replaceLastContent)
      script = REPLACE_LAST_MESSAGE;
    else if (AOptions.willAppendMoreContent) 
      script = (AOptions.isSameSender ? APPEND_NEXT_MESSAGE_NO_SCROLL : APPEND_MESSAGE_NO_SCROLL);
    else 
      script = (AOptions.isSameSender ? APPEND_NEXT_MESSAGE : APPEND_MESSAGE);
  } 
  else if (version() >= 3) 
  {
    if (AOptions.willAppendMoreContent) 
      script = (AOptions.isSameSender ? APPEND_NEXT_MESSAGE_NO_SCROLL : APPEND_MESSAGE_NO_SCROLL);
    else
      script = (AOptions.isSameSender ? APPEND_NEXT_MESSAGE : APPEND_MESSAGE);
  } 
  else if (version() >= 1) 
  {
    script = (AOptions.isSameSender ? APPEND_NEXT_MESSAGE : APPEND_MESSAGE);
  } 
  else 
  {
    script = (AOptions.isSameSender ? APPEND_NEXT_MESSAGE_WITH_SCROLL : APPEND_MESSAGE_WITH_SCROLL);
  }
  return script;
}

QString MessageStyle::loadFileData(const QString &AFileName, const QString &DefValue) const
{
  if (QFile::exists(AFileName))
  {
    QFile file(AFileName);
    if (file.open(QFile::ReadOnly))
    {
      QByteArray html = file.readAll();
      return QString::fromUtf8(html.data(),html.size());
    }
  }
  return DefValue;
}

void MessageStyle::loadTemplates()
{
  FStatusHTML =          loadFileData(FResourcePath+"/Status.html",QString::null);

  FIn_ContentHTML =      loadFileData(FResourcePath+"/Incoming/Content.html",QString::null);
  FIn_NextContentHTML =  loadFileData(FResourcePath+"/Incoming/NextContent.html",FIn_ContentHTML);
  FIn_ContextHTML =      loadFileData(FResourcePath+"/Incoming/Context.html",FIn_ContentHTML);
  FIn_NextContextHTML =  loadFileData(FResourcePath+"/Incoming/NextContext.html",FIn_ContextHTML);
  FIn_ActionHTML =       loadFileData(FResourcePath+"/Incoming/Action.html",QString::null);

  FOut_ContentHTML =     loadFileData(FResourcePath+"/Outgoing/Content.html",FIn_ContentHTML);
  FOut_NextContentHTML = loadFileData(FResourcePath+"/Outgoing/NextContent.html",FOut_ContentHTML);
  FOut_ContextHTML =     loadFileData(FResourcePath+"/Outgoing/Context.html",FOut_ContentHTML);
  FOut_NextContextHTML = loadFileData(FResourcePath+"/Outgoing/NextContext.html",FOut_ContextHTML);
  FOut_ActionHTML =      loadFileData(FResourcePath+"/Outgoing/Action.html",QString::null);
}

QList<QString> MessageStyle::styleVariants(const QString &AStylePath)
{
  QDir dir(AStylePath+"/"STYLE_RESOURCES_PATH"/Variants");
  QList<QString> files = dir.entryList(QStringList("*.css"),QDir::Files,QDir::Name);
  for (int i=0; i<files.count();i++)
    files[i].chop(4);
  return files;
}

QMap<QString, QVariant> MessageStyle::styleInfo(const QString &AStylePath)
{
  QMap<QString, QVariant> info;

  QFile file(AStylePath+"/"STYLE_CONTENTS_PATH"/Info.plist");
  if (file.open(QFile::ReadOnly))
  {
    QDomDocument doc;
    if (doc.setContent(file.readAll(),true))
    {
      QDomElement elem = doc.documentElement().firstChildElement("dict").firstChildElement("key");
      while (!elem.isNull())
      {
        QString key = elem.text();
        if (!key.isEmpty())
        {
          elem = elem.nextSiblingElement();
          if (elem.tagName() == "string")
            info.insert(key,elem.text());
          else if (elem.tagName() == "integer")
            info.insert(key,elem.text().toInt());
          else if (elem.tagName() == "true")
            info.insert(key,true);
          else if (elem.tagName() == "false")
            info.insert(key,false);
        }
        elem = elem.nextSiblingElement("key");
      }
    }
  }
  return info;
}
