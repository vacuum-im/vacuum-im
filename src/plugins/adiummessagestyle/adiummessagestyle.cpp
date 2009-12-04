#include "adiummessagestyle.h"

#include <QUrl>
#include <QDir>
#include <QFile>
#include <QRegExp>
#include <QWebFrame>
#include <QByteArray>
#include <QStringList>
#include <QWebSettings>
#include <QDomDocument>
#include <QTextDocument>
#include <QCoreApplication>

#define SHARED_STYLE_PATH                   RESOURCES_DIR"/"RSR_STORAGE_ADIUMMESSAGESTYLES"/"STORAGE_SHARED_DIR
#define STYLE_CONTENTS_PATH                 "Contents"
#define STYLE_RESOURCES_PATH                STYLE_CONTENTS_PATH"/Resources"

#define APPEND_MESSAGE_WITH_SCROLL          "checkIfScrollToBottomIsNeeded(); appendMessage(\"%1\"); scrollToBottomIfNeeded();"
#define APPEND_NEXT_MESSAGE_WITH_SCROLL	    "checkIfScrollToBottomIsNeeded(); appendNextMessage(\"%1\"); scrollToBottomIfNeeded();"
#define APPEND_MESSAGE                      "appendMessage(\"%1\");"
#define APPEND_NEXT_MESSAGE                 "appendNextMessage(\"%1\");"
#define APPEND_MESSAGE_NO_SCROLL            "appendMessageNoScroll(\"%1\");"
#define	APPEND_NEXT_MESSAGE_NO_SCROLL       "appendNextMessageNoScroll(\"%1\");"
#define REPLACE_LAST_MESSAGE                "replaceLastMessage(\"%1\");"

#define TOPIC_MAIN_DIV	                    "<div id=\"topic\"></div>"
#define TOPIC_INDIVIDUAL_WRAPPER            "<span id=\"topicEdit\" ondblclick=\"this.setAttribute('contentEditable', true); this.focus();\">%1</span>"

static const char *SenderColors[] =  {
  "aqua", "aquamarine", "blue", "blueviolet", "brown", "burlywood", "cadetblue", "chartreuse", "chocolate",
  "coral", "cornflowerblue", "crimson", "cyan", "darkblue", "darkcyan", "darkgoldenrod", "darkgreen", "darkgrey",
  "darkkhaki", "darkmagenta", "darkolivegreen", "darkorange", "darkorchid", "darkred", "darksalmon", "darkseagreen",
  "darkslateblue", "darkslategrey", "darkturquoise", "darkviolet", "deeppink", "deepskyblue", "dimgrey", "dodgerblue",
  "firebrick", "forestgreen", "fuchsia", "gold", "goldenrod", "green", "greenyellow", "grey", "hotpink", "indianred",
  "indigo", "lawngreen", "lightblue", "lightcoral", "lightgreen", "lightgrey", "lightpink", "lightsalmon",
  "lightseagreen", "lightskyblue", "lightslategrey", "lightsteelblue", "lime", "limegreen", "magenta", "maroon",
  "mediumaquamarine", "mediumblue", "mediumorchid", "mediumpurple", "mediumseagreen", "mediumslateblue",
  "mediumspringgreen", "mediumturquoise", "mediumvioletred", "midnightblue", "navy", "olive", "olivedrab", "orange",
  "orangered", "orchid", "palegreen", "paleturquoise", "palevioletred", "peru", "pink", "plum", "powderblue",
  "purple", "red", "rosybrown", "royalblue", "saddlebrown", "salmon", "sandybrown", "seagreen", "sienna", "silver",
  "skyblue", "slateblue", "slategrey", "springgreen", "steelblue", "tan", "teal", "thistle", "tomato", "turquoise",
  "violet", "yellowgreen"
};

static int SenderColorsCount = sizeof(SenderColors)/sizeof(SenderColors[0]);

AdiumMessageStyle::AdiumMessageStyle(const QString &AStylePath, QObject *AParent) : QObject(AParent)
{
  FInfo = styleInfo(AStylePath);
  FVariants = styleVariants(AStylePath);
  FResourcePath = AStylePath+"/"STYLE_RESOURCES_PATH;
  initStyleSettings();
  loadTemplates();
  loadSenderColors();
  connect(AParent,SIGNAL(styleWidgetAdded(IMessageStyle *, QWidget *)),SLOT(onStyleWidgetAdded(IMessageStyle *, QWidget *)));
}

AdiumMessageStyle::~AdiumMessageStyle()
{

}

bool AdiumMessageStyle::isValid() const
{
  return !FIn_ContentHTML.isEmpty() && !styleId().isEmpty();
}

QString AdiumMessageStyle::styleId() const
{
  return FInfo.value(MSIV_NAME).toString();
}

QList<QWidget *> AdiumMessageStyle::styleWidgets() const
{
  return FWidgetStatus.keys();
}

QWidget *AdiumMessageStyle::createWidget(const IMessageStyleOptions &AOptions, QWidget *AParent)
{
  StyleViewer *view = new StyleViewer(AParent);
  view->page()->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);
  changeOptions(view,AOptions,true);
  return view;
}

QString AdiumMessageStyle::senderColor(const QString &ASenderId) const
{
  if (!FSenderColors.isEmpty())
    return FSenderColors.at(qHash(ASenderId) % FSenderColors.count());
  return QString(SenderColors[qHash(ASenderId) % SenderColorsCount]);
}

bool AdiumMessageStyle::changeOptions(QWidget *AWidget, const IMessageStyleOptions &AOptions, bool AClean)
{
  StyleViewer *view = qobject_cast<StyleViewer *>(AWidget);
  if (view && AOptions.extended.value(MSO_STYLE_ID).toString()==styleId())
  {
    if (!FWidgetStatus.contains(AWidget))
    {
      FWidgetStatus[view].lastKind = -1;
      connect(view,SIGNAL(linkClicked(const QUrl &)),SLOT(onLinkClicked(const QUrl &)));
      connect(view,SIGNAL(destroyed(QObject *)),SLOT(onStyleWidgetDestroyed(QObject *)));
      emit widgetAdded(AWidget);
    }
    else
    {
      FWidgetStatus[view].lastKind = -1;
    }

    if (AClean)
    {
      QString html = makeStyleTemplate(AOptions);
      fillStyleKeywords(html,AOptions);
      view->setHtml(html);
    }
    else
    {
      setVariant(AWidget,AOptions.extended.value(MSO_VARIANT).toString());
    }
    
    int fontSize = AOptions.extended.value(MSO_FONT_SIZE).toInt();
    QString fontFamily = AOptions.extended.value(MSO_FONT_FAMILY).toString();
    view->page()->settings()->setFontSize(QWebSettings::DefaultFontSize, fontSize!=0 ? fontSize : QWebSettings::globalSettings()->fontSize(QWebSettings::DefaultFontSize));
    view->page()->settings()->setFontFamily(QWebSettings::StandardFont, !fontFamily.isEmpty() ? fontFamily : QWebSettings::globalSettings()->fontFamily(QWebSettings::StandardFont));

    emit optionsChanged(AWidget,AOptions,AClean);
    return true;
  }
  return false;
}

bool AdiumMessageStyle::appendContent(QWidget *AWidget, const QString &AHtml, const IMessageContentOptions &AOptions)
{
  StyleViewer *view = FWidgetStatus.contains(AWidget) ? qobject_cast<StyleViewer *>(AWidget) : NULL;
  if (view)
  {
    bool sameSender = isSameSender(AWidget,AOptions);
    QString html = makeContentTemplate(AOptions,sameSender);
    fillContentKeywords(html,AOptions,sameSender);

    html.replace("%message%",AHtml);
    if (AOptions.kind == IMessageContentOptions::Topic)
      html.replace("%topic%",QString(TOPIC_INDIVIDUAL_WRAPPER).arg(AHtml));

    escapeStringForScript(html);
    view->page()->mainFrame()->evaluateJavaScript(scriptForAppendContent(sameSender,AOptions.noScroll).arg(html));

    WidgetStatus &wstatus = FWidgetStatus[AWidget];
    wstatus.lastKind = AOptions.kind;
    wstatus.lastId = AOptions.senderId;
    wstatus.lastTime = AOptions.time;

    emit contentAppended(AWidget,AHtml,AOptions);
    return true;
  }
  return false;
}

int AdiumMessageStyle::version() const
{
  return FInfo.value(MSIV_VERSION,0).toInt();
}

QMap<QString, QVariant> AdiumMessageStyle::infoValues() const
{
  return FInfo;
}

QList<QString> AdiumMessageStyle::variants() const
{
  return FVariants;
}

QList<QString> AdiumMessageStyle::styleVariants(const QString &AStylePath)
{
  QList<QString> files;
  if (!AStylePath.isEmpty())
  {
    QDir dir(AStylePath+"/"STYLE_RESOURCES_PATH"/Variants");
    files = dir.entryList(QStringList("*.css"),QDir::Files,QDir::Name);
    for (int i=0; i<files.count();i++)
      files[i].chop(4);
  }
  return files;
}

QMap<QString, QVariant> AdiumMessageStyle::styleInfo(const QString &AStylePath)
{
  QMap<QString, QVariant> info;

  QFile file(AStylePath+"/"STYLE_CONTENTS_PATH"/Info.plist");
  if (!AStylePath.isEmpty() && file.open(QFile::ReadOnly))
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

bool AdiumMessageStyle::isSameSender(QWidget *AWidget, const IMessageContentOptions &AOptions) const
{
  if (!FCombineConsecutive)
    return false;
  if (AOptions.senderId.isEmpty())
    return false;

  const WidgetStatus &wstatus = FWidgetStatus.value(AWidget);
  if (wstatus.lastKind != AOptions.kind)
    return false;
  if (wstatus.lastId != AOptions.senderId)
    return false;
  if (wstatus.lastTime.secsTo(AOptions.time)>2*60)
    return false;

  return true;
}

void AdiumMessageStyle::setVariant(QWidget *AWidget, const QString &AVariant)
{
  StyleViewer *view = FWidgetStatus.contains(AWidget) ? qobject_cast<StyleViewer *>(AWidget) : NULL;
  if (view)
  {
    QString variant = FResourcePath+"/"+QDir::cleanPath(QString("Variants/%1.css").arg(!FVariants.contains(AVariant) ? FInfo.value(MSIV_DEFAULT_VARIANT,"../main").toString() : AVariant));
    escapeStringForScript(variant);
    QString script = QString("setStylesheet(\"%1\",\"%2\");").arg("mainStyle").arg(variant);
    view->page()->mainFrame()->evaluateJavaScript(script);
  }
}

QString AdiumMessageStyle::makeStyleTemplate(const IMessageStyleOptions &AOptions) const
{
  bool usingCustomTemplate = true;
  QString htmlFileName = FResourcePath+"/Template.html";
  if (!QFile::exists(htmlFileName))
  {
    usingCustomTemplate = false;
    htmlFileName = qApp->applicationDirPath()+"/"SHARED_STYLE_PATH"/Template.html";
  }

  QString html = loadFileData(htmlFileName,QString::null);
  if (!html.isEmpty())
  {
    QString headerHTML;
    if (AOptions.extended.value(MSO_HEADER_TYPE).toInt() == AdiumMessageStyle::HeaderTopic)
      headerHTML = TOPIC_MAIN_DIV;
    else if (AOptions.extended.value(MSO_HEADER_TYPE).toInt() == AdiumMessageStyle::HeaderNormal)
      headerHTML =  loadFileData(FResourcePath+"/Header.html",QString::null);
    QString footerHTML = loadFileData(FResourcePath+"/Footer.html",QString::null);

    QString variant = AOptions.extended.value(MSO_VARIANT).toString();
    if (!FVariants.contains(variant))
      variant = FInfo.value(MSIV_DEFAULT_VARIANT,"../main").toString();
    variant = QDir::cleanPath(QString("Variants/%1.css").arg(variant));

    html.replace(html.indexOf("%@"),2,FResourcePath+"/");
    if (!usingCustomTemplate || version()>=3)
      html.replace(html.indexOf("%@"),2, version()>=3 ? "@import url( \"main.css\" );" : "");
    html.replace(html.indexOf("%@"),2,FResourcePath+"/"+variant);
    html.replace(html.indexOf("%@"),2,headerHTML);
    html.replace(html.indexOf("%@"),2,footerHTML);
  }
  return html;
}

void AdiumMessageStyle::fillStyleKeywords(QString &AHtml, const IMessageStyleOptions &AOptions) const
{
  AHtml.replace("%chatName%",AOptions.extended.value(MSO_CHAT_NAME).toString());
  AHtml.replace("%sourceName%",AOptions.extended.value(MSO_ACCOUNT_NAME).toString());
  AHtml.replace("%destinationName%",AOptions.extended.value(MSO_CHAT_NAME).toString());
  AHtml.replace("%destinationDisplayName%",AOptions.extended.value(MSO_CHAT_NAME).toString());
  AHtml.replace("%outgoingIconPath%",AOptions.extended.value(MSO_SELF_AVATAR,FResourcePath+"/outgoing_icon.png").toString());
  AHtml.replace("%incomingIconPath%",AOptions.extended.value(MSO_CONTACT_AVATAR,FResourcePath+"/incoming_icon.png").toString());
  AHtml.replace("%timeOpened%",Qt::escape(AOptions.extended.value(MSO_START_TIME).toDateTime().toString()));
  AHtml.replace("%serviceIconImg%", "");

  QString background;
  if (FAllowCustomBackground)
  {
    if (!AOptions.extended.value(MSO_BG_IMAGE_FILE).toString().isEmpty())
    {
      int imageLayout = AOptions.extended.value(MSO_BG_IMAGE_LAYOUT).toInt();
      if (imageLayout == ImageNormal)
        background.append("background-image: url('%1'); background-repeat: no-repeat; background-attachment:fixed;");
      else if (imageLayout == ImageCenter)
        background.append("background-image: url('%1'); background-position: center; background-repeat: no-repeat; background-attachment:fixed;");
      else if (imageLayout == ImageTitle)
        background.append("background-image: url('%1'); background-repeat: repeat;");
      else if (imageLayout == ImageTitleCenter)
        background.append("background-image: url('%1'); background-repeat: repeat; background-position: center;");
      else if (imageLayout == ImageScale)
        background.append("background-image: url('%1'); -webkit-background-size: 100% 100%; background-size: 100% 100%; background-attachment: fixed;");
      background = background.arg(AOptions.extended.value(MSO_BG_IMAGE_FILE).toString());
    }
    if (!AOptions.extended.value(MSO_BG_COLOR).toString().isEmpty())
    {
      QColor color(AOptions.extended.value(MSO_BG_COLOR).toString());
      if (!color.isValid())
        color.setNamedColor("#"+AOptions.extended.value(MSO_BG_COLOR).toString());
      if (color.isValid())
      {
        int r,g,b,a;
        color.getRgb(&r,&g,&b,&a);
        background.append(QString("background-color: rgba(%1, %2, %3, %4);").arg(r).arg(g).arg(b).arg(qreal(a)/255.0));
      }
    }
  }
  AHtml.replace("==bodyBackground==", background);
}

QString AdiumMessageStyle::makeContentTemplate(const IMessageContentOptions &AOptions, bool ASameSender) const
{
  QString html;
  if (false && AOptions.kind == IMessageContentOptions::Topic && !FTopicHTML.isEmpty())
  {
    html = FTopicHTML;
  }
  else if (AOptions.kind == IMessageContentOptions::Status && !FStatusHTML.isEmpty())
  {
    html = FStatusHTML;
  }
  else
  {
    if (AOptions.type & IMessageContentOptions::History)
    {
      if (AOptions.direction == IMessageContentOptions::DirectionIn)
        html = ASameSender ? FIn_NextContextHTML : FIn_ContextHTML;
      else
        html = ASameSender ? FOut_NextContextHTML : FOut_ContextHTML;
    }
    else if (AOptions.direction == IMessageContentOptions::DirectionIn)
    {
      html = ASameSender ? FIn_NextContentHTML : FIn_ContentHTML;
    }
    else
    {
      html = ASameSender ? FOut_NextContentHTML : FOut_ContentHTML;
    }
  }
  return html;
}

void AdiumMessageStyle::fillContentKeywords(QString &AHtml, const IMessageContentOptions &AOptions, bool ASameSender) const
{
  bool isDirectionIn = AOptions.direction == IMessageContentOptions::DirectionIn;

  QStringList messageClasses;
  if (FCombineConsecutive && ASameSender)
    messageClasses << MSMC_CONSECUTIVE;

  if (AOptions.kind == IMessageContentOptions::Status)
    messageClasses << MSMC_STATUS;
  else
    messageClasses << MSMC_MESSAGE;

  if (AOptions.type & IMessageContentOptions::Groupchat)
    messageClasses << MSMC_GROUPCHAT;
  if (AOptions.type & IMessageContentOptions::History)
    messageClasses << MSMC_HISTORY;
  if (AOptions.type & IMessageContentOptions::Event)
    messageClasses << MSMC_EVENT;
  if (AOptions.type & IMessageContentOptions::Mention)
    messageClasses << MSMC_MENTION;
  if (AOptions.type & IMessageContentOptions::Notification)
    messageClasses << MSMC_NOTIFICATION;

  if (isDirectionIn)
    messageClasses << MSMC_INCOMING;
  else
    messageClasses << MSMC_OUTGOING;

  AHtml.replace("%messageClasses%", messageClasses.join(" "));

  //AHtml.replace("%messageDirection%", AOptions.isAlignLTR ? "ltr" : "rtl" );
  AHtml.replace("%senderStatusIcon%",AOptions.senderIcon);
  AHtml.replace("%shortTime%", Qt::escape(AOptions.time.toString(tr("hh:mm"))));
  AHtml.replace("%service%","");

  QString avatar = AOptions.senderAvatar;
  if (!QFile::exists(avatar))
  {
    avatar = FResourcePath+(isDirectionIn ? "/Incoming/buddy_icon.png" : "/Outgoing/buddy_icon.png");
    if (!isDirectionIn && !QFile::exists(avatar))
      avatar = FResourcePath+"/Incoming/buddy_icon.png";
  }
  AHtml.replace("%userIconPath%",avatar);

  QString timeFormat = !AOptions.timeFormat.isEmpty() ? AOptions.timeFormat : tr("hh:mm:ss");
  QString time = Qt::escape(AOptions.time.toString(timeFormat));
  AHtml.replace("%time%", time);

  QRegExp timeRegExp("%time\\{([^}]*)\\}%");
  for (int pos=0; pos!=-1; pos = timeRegExp.indexIn(AHtml, pos))
    if (!timeRegExp.cap(0).isEmpty())
      AHtml.replace(pos, timeRegExp.cap(0).length(), time);

  QString sColor = !AOptions.senderColor.isEmpty() ? AOptions.senderColor : senderColor(AOptions.senderId);
  AHtml.replace("%senderColor%",sColor);

  QRegExp scolorRegExp("%senderColor\\{([^}]*)\\}%");
  for (int pos=0; pos!=-1; pos = scolorRegExp.indexIn(AHtml, pos))
    if (!scolorRegExp.cap(0).isEmpty())
      AHtml.replace(pos, scolorRegExp.cap(0).length(), sColor);

  if (AOptions.kind == IMessageContentOptions::Status)
  {
    AHtml.replace("%status%","");
    AHtml.replace("%statusSender%",AOptions.senderName);
  }
  else
  {
    AHtml.replace("%senderScreenName%",AOptions.senderId);
    AHtml.replace("%sender%",AOptions.senderName);
    AHtml.replace("%senderDisplayName%",AOptions.senderName);
    AHtml.replace("%senderPrefix%","");

    QString rgbaColor;
    QColor bgColor(AOptions.textBGColor);
    QRegExp colorRegExp("%textbackgroundcolor\\{([^}]*)\\}%");
    for (int pos=0; pos!=-1; pos = colorRegExp.indexIn(AHtml, pos))
    {
      if (!colorRegExp.cap(0).isEmpty())
      {
        if (bgColor.isValid())
        {
          int r,g,b;
          bool ok = false;
          qreal a = colorRegExp.cap(1).toDouble(&ok);
          bgColor.setAlphaF(ok ? a : 1.0);
          bgColor.getRgb(&r,&g,&b);
          rgbaColor = QString("rgba(%1, %2, %3, %4)").arg(r).arg(g).arg(b).arg(a);
        }
        else if (rgbaColor.isEmpty())
        {
          rgbaColor = "inherit";
        }
        AHtml.replace(pos, colorRegExp.cap(0).length(), rgbaColor);
      }
    }
  }
}

void AdiumMessageStyle::escapeStringForScript(QString &AText) const
{
  AText.replace("\\","\\\\");
  AText.replace("\"","\\\"");
  AText.replace("\n","");
  AText.replace("\r","<br>");
}

QString AdiumMessageStyle::scriptForAppendContent(bool ASameSender, bool ANoScroll) const
{
  QString script;

  if (version() >= 4) 
  {
    if (ANoScroll) 
      script = (FCombineConsecutive && ASameSender ? APPEND_NEXT_MESSAGE_NO_SCROLL : APPEND_MESSAGE_NO_SCROLL);
    else 
      script = (FCombineConsecutive && ASameSender ? APPEND_NEXT_MESSAGE : APPEND_MESSAGE);
  } 
  else if (version() >= 3) 
  {
    if (ANoScroll) 
      script = (FCombineConsecutive && ASameSender ? APPEND_NEXT_MESSAGE_NO_SCROLL : APPEND_MESSAGE_NO_SCROLL);
    else
      script = (FCombineConsecutive && ASameSender ? APPEND_NEXT_MESSAGE : APPEND_MESSAGE);
  } 
  else if (version() >= 1) 
  {
    script = (ASameSender ? APPEND_NEXT_MESSAGE : APPEND_MESSAGE);
  } 
  else 
  {
    script = (ASameSender ? APPEND_NEXT_MESSAGE_WITH_SCROLL : APPEND_MESSAGE_WITH_SCROLL);
  }
  return script;
}

QString AdiumMessageStyle::loadFileData(const QString &AFileName, const QString &DefValue) const
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

void AdiumMessageStyle::loadTemplates()
{
  FIn_ContentHTML =      loadFileData(FResourcePath+"/Incoming/Content.html",QString::null);
  FIn_NextContentHTML =  loadFileData(FResourcePath+"/Incoming/NextContent.html",FIn_ContentHTML);
  FIn_ContextHTML =      loadFileData(FResourcePath+"/Incoming/Context.html",FIn_ContentHTML);
  FIn_NextContextHTML =  loadFileData(FResourcePath+"/Incoming/NextContext.html",FIn_ContextHTML);

  FOut_ContentHTML =     loadFileData(FResourcePath+"/Outgoing/Content.html",FIn_ContentHTML);
  FOut_NextContentHTML = loadFileData(FResourcePath+"/Outgoing/NextContent.html",FOut_ContentHTML);
  FOut_ContextHTML =     loadFileData(FResourcePath+"/Outgoing/Context.html",FOut_ContentHTML);
  FOut_NextContextHTML = loadFileData(FResourcePath+"/Outgoing/NextContext.html",FOut_ContextHTML);

  FStatusHTML =          loadFileData(FResourcePath+"/Status.html",FIn_ContentHTML);
  FTopicHTML =           loadFileData(FResourcePath+"/Topic.html",QString::null);
}

void AdiumMessageStyle::loadSenderColors()
{
  QFile colors(FResourcePath + "/Incoming/SenderColors.txt");
  if (colors.open(QFile::ReadOnly))
    FSenderColors = QString::fromUtf8(colors.readAll()).split(':',QString::SkipEmptyParts);
}

void AdiumMessageStyle::initStyleSettings()
{
  FCombineConsecutive = !FInfo.value(MSIV_DISABLE_COMBINE_CONSECUTIVE,false).toBool();
  FAllowCustomBackground = !FInfo.value(MSIV_DISABLE_CUSTOM_BACKGROUND,false).toBool();
}

void AdiumMessageStyle::onLinkClicked(const QUrl &AUrl)
{
  StyleViewer *view = qobject_cast<StyleViewer *>(sender());
  emit urlClicked(view,AUrl);
}

void AdiumMessageStyle::onStyleWidgetAdded(IMessageStyle *AStyle, QWidget *AWidget)
{
  if (AStyle!=this && FWidgetStatus.contains(AWidget))
  {
    FWidgetStatus.remove(AWidget);
    emit widgetRemoved(AWidget);
  }
}

void AdiumMessageStyle::onStyleWidgetDestroyed(QObject *AObject)
{
  FWidgetStatus.remove((QWidget *)AObject);
  emit widgetRemoved((QWidget *)AObject);
}
