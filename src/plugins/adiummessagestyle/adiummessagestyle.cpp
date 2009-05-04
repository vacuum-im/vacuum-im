#include "adiummessagestyle.h"

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
}

AdiumMessageStyle::~AdiumMessageStyle()
{

}

bool AdiumMessageStyle::isValid() const
{
  return !FIn_ContentHTML.isEmpty();
}

QString AdiumMessageStyle::styleId() const
{
  return FInfo.value(MSIV_NAME).toString();
}

int AdiumMessageStyle::version() const
{
  return FInfo.value(MSIV_VERSION,0).toInt();
}

QList<QString> AdiumMessageStyle::variants() const
{
  return FVariants;
}

QWidget *AdiumMessageStyle::styleWidget(const IMessageStyleOptions &AOptions, QWidget *AParent)
{
  QString html = makeStyleTemplate(AOptions);
  fillStyleKeywords(html,AOptions);
  QWebView *widget = new QWebView(AParent);
  widget->setHtml(html);
  emit styleSeted(widget,AOptions);
  return widget;
}

void AdiumMessageStyle::setVariant(QWebView *AView, const QString &AVariant) const
{
  QString variant = QDir::cleanPath(QString("Variants/%1.css").arg(!FVariants.contains(AVariant) ? FInfo.value("DefaultVariant","../main").toString() : AVariant));
  QString script = QString("setStylesheet(\"%1\",\"%2\");").arg("mainStyle").arg(FResourcePath+"/"+variant);
  escapeStringForScript(script);
  AView->page()->mainFrame()->evaluateJavaScript(script);
  emit variantSeted(AView,AVariant);
}

void AdiumMessageStyle::appendContent(QWebView *AView, const QString &AMessage, const IMessageStyle::ContentOptions &AOptions) const
{
  QString html = makeContentTemplate(AOptions);
  fillContentKeywords(html,AOptions);

  html.replace("%message%",AMessage);
  if (AOptions.contentType == IMessageStyle::ContentTopic)
    html.replace("%topic%",QString(TOPIC_INDIVIDUAL_WRAPPER).arg(AMessage));

  escapeStringForScript(html);
  AView->page()->mainFrame()->evaluateJavaScript(scriptForAppendContent(AOptions).arg(html));

  emit contentAppended(AView,AMessage,AOptions);
}

QMap<QString, QVariant> AdiumMessageStyle::infoValues() const
{
  return FInfo;
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
    IAdiumMessageStyleOptions options;
    if (AOptions.options)
      options = *((IAdiumMessageStyleOptions *)AOptions.options);

    QString headerHTML;
    if (false && options.headerType == IMessageStyle::HeaderTopic)
      headerHTML = TOPIC_MAIN_DIV;
    else if (options.headerType == IMessageStyle::HeaderNormal)
      headerHTML =  loadFileData(FResourcePath+"/Header.html",QString::null);
    QString footerHTML = loadFileData(FResourcePath+"/Footer.html",QString::null);
    QString variant = QDir::cleanPath(QString("Variants/%1.css").arg(!FVariants.contains(AOptions.variant) ? FInfo.value("DefaultVariant","../main").toString() : AOptions.variant));

    html.replace(html.indexOf("%@"),2,FResourcePath+"/");
    if (!usingCustomTemplate || version()>=3)
      html.replace(html.indexOf("%@"),2,version()<3 ? "" : "@import url( \"main.css\" );");
    html.replace(html.indexOf("%@"),2,FResourcePath+"/"+variant);
    html.replace(html.indexOf("%@"),2,headerHTML);
    html.replace(html.indexOf("%@"),2,footerHTML);
  }
  return html;
}

QString AdiumMessageStyle::makeContentTemplate(const IMessageStyle::ContentOptions &AOptions) const
{
  QString html;
  if (false && AOptions.contentType == IMessageStyle::ContentTopic && !FTopicHTML.isEmpty())
  {
    html = FTopicHTML;
  }
  else if (AOptions.contentType == IMessageStyle::ContentStatus && !FStatusHTML.isEmpty())
  {
    html = FStatusHTML;
  }
  else if (AOptions.contentType == IMessageStyle::ContentArchive)
  {
    if (AOptions.isDirectionIn)
      html = FCombineConsecutive && AOptions.isSameSender ? FIn_NextContextHTML : FIn_ContextHTML;
    else
      html = FCombineConsecutive && AOptions.isSameSender ? FOut_NextContextHTML : FOut_ContextHTML;
  }
  else
  {
    if (AOptions.isDirectionIn)
      html = FCombineConsecutive && AOptions.isSameSender ? FIn_NextContentHTML : FIn_ContentHTML;
    else
      html = FCombineConsecutive && AOptions.isSameSender ? FOut_NextContentHTML : FOut_ContentHTML;
  }
  return html;
}

void AdiumMessageStyle::fillStyleKeywords(QString &AHtml, const IMessageStyleOptions &AOptions) const
{
  IAdiumMessageStyleOptions options;
  if (AOptions.options)
    options = *((IAdiumMessageStyleOptions *)AOptions.options);

  AHtml.replace("%chatName%",options.chatName);
  AHtml.replace("%sourceName%", options.accountName);
  AHtml.replace("%destinationName%", options.chatName);
  AHtml.replace("%destinationDisplayName%", options.chatName);
  AHtml.replace("%incomingIconPath%", !options.contactAvatar.isEmpty() ? options.contactAvatar : FResourcePath+"/incoming_icon.png");
  AHtml.replace("%outgoingIconPath%", !options.selfAvatar.isEmpty() ? options.selfAvatar : FResourcePath+"/outgoing_icon.png");
  AHtml.replace("%timeOpened%", Qt::escape(options.startTime.toString()));
  AHtml.replace("%serviceIconImg%", "");

  QString background;
  if (FAllowCustomBackground)
  {
    if (!options.bgImageFile.isEmpty())
    {
      if (options.bgImageLayout == IMessageStyle::ImageNormal)
        background.append("background-image: url('%1'); background-repeat: no-repeat; background-attachment:fixed;");
      else if (options.bgImageLayout == IMessageStyle::ImageCenter)
        background.append("background-image: url('%1'); background-position: center; background-repeat: no-repeat; background-attachment:fixed;");
      else if (options.bgImageLayout == IMessageStyle::ImageTitle)
        background.append("background-image: url('%1'); background-repeat: repeat;");
      else if (options.bgImageLayout == IMessageStyle::ImageTitleCenter)
        background.append("background-image: url('%1'); background-repeat: repeat; background-position: center;");
      else if (options.bgImageLayout == IMessageStyle::ImageScale)
        background.append("background-image: url('%1'); -webkit-background-size: 100% 100%; background-size: 100% 100%; background-attachment: fixed;");
      background = background.arg(options.bgImageFile);
    }
    if (options.bgColor.isValid())
    {
      int r,g,b,a;
      options.bgColor.getRgb(&r,&g,&b,&a);
      background.append(QString("background-color: rgba(%1, %2, %3, %4);").arg(r).arg(g).arg(b).arg(qreal(a)/255.0));
    }
  }
  AHtml.replace("==bodyBackground==", background);
}

void AdiumMessageStyle::fillContentKeywords(QString &AHtml, const IMessageStyle::ContentOptions &AOptions) const
{
  AHtml.replace("%senderStatusIcon%",AOptions.senderStatusIcon);
  AHtml.replace("%messageClasses%", (FCombineConsecutive && AOptions.isSameSender ? MSMC_CONSECUTIVE" " : "") + AOptions.messageClasses.join(" "));
  AHtml.replace("%messageDirection%", AOptions.isAlignLTR ? "ltr" : "rtl" );
  AHtml.replace("%shortTime%", Qt::escape(AOptions.sendTime.toString(tr("hh:mm"))));
  AHtml.replace("%service%","");

  if (!AOptions.senderAvatar.isEmpty())
    AHtml.replace("%userIconPath%",AOptions.senderAvatar);
  else
    AHtml.replace("%userIconPath%",FResourcePath+(AOptions.isDirectionIn ? "/Incoming/buddy_icon.png" : "/Outgoing/buddy_icon.png"));

  QString timeFormat = !AOptions.sendTimeFormat.isEmpty() ? AOptions.sendTimeFormat : tr("hh:mm:ss");
  QString time = Qt::escape(AOptions.sendTime.toString(timeFormat));
  AHtml.replace("%time%", time);
  QRegExp timeRegExp("%time\\{([^}]*)\\}%");
  for (int pos=0; pos!=-1; pos = timeRegExp.indexIn(AHtml, pos))
    if (!timeRegExp.cap(0).isEmpty())
      AHtml.replace(pos, timeRegExp.cap(0).length(), time);

  QString sColor = AOptions.senderColor;
  if (sColor.isEmpty())
  {
    if (FSenderColors.isEmpty())
      sColor = QString(SenderColors[qHash(AOptions.senderName) % SenderColorsCount]);
    else
      sColor = FSenderColors.at(qHash(AOptions.senderName) % FSenderColors.count());
  }
  AHtml.replace("%senderColor%",sColor);
  QRegExp scolorRegExp("%senderColor\\{([^}]*)\\}%");
  for (int pos=0; pos!=-1; pos = scolorRegExp.indexIn(AHtml, pos))
    if (!scolorRegExp.cap(0).isEmpty())
      AHtml.replace(pos, scolorRegExp.cap(0).length(), sColor);

  if (AOptions.contentType == IMessageStyle::ContentStatus)
  {
    AHtml.replace("%status%",AOptions.statusKeyword);
    AHtml.replace("%statusSender%",AOptions.senderName);
  }
  else
  {
    AHtml.replace("%sender%",AOptions.senderName);
    AHtml.replace("%senderScreenName%",AOptions.senderName);
    AHtml.replace("%senderDisplayName%",AOptions.senderName);
    AHtml.replace("%senderPrefix%","");

    QString rgbaColor;
    QColor bgColor(AOptions.textBGColor);
    QRegExp colorRegExp("%textbackgroundcolor\\{([^}]*)\\}%");
    for (int pos=0; pos!=-1; pos = colorRegExp.indexIn(AHtml, pos))
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

void AdiumMessageStyle::escapeStringForScript(QString &AText) const
{
  AText.replace("\\","\\\\");
  AText.replace("\"","\\\"");
  AText.replace("\n","");
  AText.replace("\r","<br>");
}

QString AdiumMessageStyle::scriptForAppendContent(const IMessageStyle::ContentOptions &AOptions) const
{
  QString script;

  if (version() >= 4) 
  {
    if (AOptions.replaceLastContent)
      script = REPLACE_LAST_MESSAGE;
    else if (AOptions.willAppendMoreContent) 
      script = (FCombineConsecutive && AOptions.isSameSender ? APPEND_NEXT_MESSAGE_NO_SCROLL : APPEND_MESSAGE_NO_SCROLL);
    else 
      script = (FCombineConsecutive && AOptions.isSameSender ? APPEND_NEXT_MESSAGE : APPEND_MESSAGE);
  } 
  else if (version() >= 3) 
  {
    if (AOptions.willAppendMoreContent) 
      script = (FCombineConsecutive && AOptions.isSameSender ? APPEND_NEXT_MESSAGE_NO_SCROLL : APPEND_MESSAGE_NO_SCROLL);
    else
      script = (FCombineConsecutive && AOptions.isSameSender ? APPEND_NEXT_MESSAGE : APPEND_MESSAGE);
  } 
  else if (version() >= 1) 
  {
    script = (FCombineConsecutive && AOptions.isSameSender ? APPEND_NEXT_MESSAGE : APPEND_MESSAGE);
  } 
  else 
  {
    script = (FCombineConsecutive && AOptions.isSameSender ? APPEND_NEXT_MESSAGE_WITH_SCROLL : APPEND_MESSAGE_WITH_SCROLL);
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
