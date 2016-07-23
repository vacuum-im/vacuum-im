#include "simplemessagestyle.h"

#include <QDir>
#include <QFile>
#include <QScrollBar>
#include <QTextFrame>
#include <QTextCursor>
#include <QDomDocument>
#include <QCoreApplication>
#include <QTextDocumentFragment>
#include <definitions/resources.h>
#include <definitions/optionvalues.h>
#include <utils/filestorage.h>
#include <utils/textmanager.h>
#include <utils/options.h>
#include <utils/logger.h>

#define CONSECUTIVE_TIMEOUT                 2*60

#define SCROLL_TIMEOUT                      100
#define SHARED_STYLE_PATH                   RESOURCES_DIR "/" RSR_STORAGE_SIMPLEMESSAGESTYLES "/" FILE_STORAGE_SHARED_DIR

static const char *SenderColors[] =  {
	"blue", "blueviolet", "brown", "cadetblue", "chocolate", "coral", "cornflowerblue", "crimson",
	"darkblue", "darkcyan", "darkgoldenrod", "darkgreen", "darkmagenta", "darkolivegreen", "darkorange",
	"darkorchid", "darkred", "darksalmon", "darkslateblue", "darkslategrey", "darkturquoise", "darkviolet",
	"deeppink", "deepskyblue", "dodgerblue", "firebrick", "forestgreen", "fuchsia", "gold", "green",
	"hotpink", "indianred", "indigo", "lightcoral", "lightseagreen", "limegreen", "magenta", "maroon",
	"mediumblue", "mediumorchid", "mediumpurple", "mediumseagreen", "mediumslateblue", "mediumvioletred",
	"midnightblue", "navy", "olive", "olivedrab", "orange", "orangered", "orchid", "palevioletred", "peru",
	"purple", "red", "rosybrown", "royalblue", "saddlebrown", "salmon", "seagreen", "sienna", "slateblue",
	"steelblue", "teal", "tomato", "violet"
};
static int SenderColorsCount = sizeof(SenderColors)/sizeof(SenderColors[0]);

QString SimpleMessageStyle::FSharedPath = QString::null;

SimpleMessageStyle::SimpleMessageStyle(const QString &AStylePath, QNetworkAccessManager *ANetworkAccessManager, QObject *AParent) : QObject(AParent)
{
	if (FSharedPath.isEmpty())
	{
		if (QDir::isRelativePath(SHARED_STYLE_PATH))
			FSharedPath = qApp->applicationDirPath()+"/"+SHARED_STYLE_PATH;
		else
			FSharedPath = SHARED_STYLE_PATH;
	}

	FStylePath = AStylePath;
	FInfo = styleInfo(AStylePath);
	FVariants = styleVariants(AStylePath);
	FNetworkAccessManager = ANetworkAccessManager;

	FScrollTimer.setSingleShot(true);
	FScrollTimer.setInterval(SCROLL_TIMEOUT);
	connect(&FScrollTimer,SIGNAL(timeout()),SLOT(onScrollAfterResize()));

	connect(AParent,SIGNAL(styleWidgetAdded(IMessageStyle *, QWidget *)),SLOT(onStyleWidgetAdded(IMessageStyle *, QWidget *)));

	initStyleSettings();
	loadTemplates();
	loadSenderColors();
}

SimpleMessageStyle::~SimpleMessageStyle()
{

}

bool SimpleMessageStyle::isValid() const
{
	return !FIn_ContentHTML.isEmpty() && !styleId().isEmpty();
}

QString SimpleMessageStyle::styleId() const
{
	return FInfo.value(MSIV_NAME).toString();
}

QList<QWidget *> SimpleMessageStyle::styleWidgets() const
{
	return FWidgetStatus.keys();
}

QWidget *SimpleMessageStyle::createWidget(const IMessageStyleOptions &AOptions, QWidget *AParent)
{
	StyleViewer *view = new StyleViewer(AParent);
	if (FNetworkAccessManager)
		view->setNetworkAccessManager(FNetworkAccessManager);
	changeOptions(view,AOptions,true);
	return view;
}

QString SimpleMessageStyle::senderColorById(const QString &ASenderId) const
{
	if (!FSenderColors.isEmpty())
		return FSenderColors.at(qHash(ASenderId) % FSenderColors.count());
	else
		return QString(SenderColors[qHash(ASenderId) % SenderColorsCount]);
}

QTextDocumentFragment SimpleMessageStyle::selection(QWidget *AWidget) const
{
	StyleViewer *view = qobject_cast<StyleViewer *>(AWidget);
	return view!=NULL ? view->textCursor().selection() : QTextDocumentFragment();
}

QTextCharFormat SimpleMessageStyle::textFormatAt(QWidget *AWidget, const QPoint &APosition) const
{
	StyleViewer *view = qobject_cast<StyleViewer *>(AWidget);
	return view!=NULL ? view->cursorForPosition(APosition).charFormat() : QTextCharFormat();
}

QTextDocumentFragment SimpleMessageStyle::textFragmentAt(QWidget *AWidget, const QPoint &APosition) const
{
	StyleViewer *view = qobject_cast<StyleViewer *>(AWidget);
	if (view)
	{
		QTextCursor cursor = view->cursorForPosition(APosition);
		for (QTextBlock::iterator it = cursor.block().begin(); !it.atEnd(); ++it)
		{
			if (it.fragment().contains(cursor.position()))
			{
				cursor.setPosition(it.fragment().position());
				cursor.movePosition(QTextCursor::NextCharacter,QTextCursor::KeepAnchor,it.fragment().length());
				return cursor.selection();
			}
		}
	}
	return QTextDocumentFragment();
}

bool SimpleMessageStyle::changeOptions(QWidget *AWidget, const IMessageStyleOptions &AOptions, bool AClear)
{
	StyleViewer *view = qobject_cast<StyleViewer *>(AWidget);
	if (view && AOptions.styleId==styleId())
	{
		bool isNewView = !FWidgetStatus.contains(view);
		if (AClear || isNewView)
		{
			WidgetStatus &wstatus = FWidgetStatus[view];
			wstatus.lastKind = -1;
			wstatus.lastId = QString::null;
			wstatus.lastTime = QDateTime();
			wstatus.scrollStarted = false;
			wstatus.content.clear();
			wstatus.options = AOptions.extended;

			if (isNewView)
			{
				view->installEventFilter(this);
				connect(view,SIGNAL(anchorClicked(const QUrl &)),SLOT(onStyleWidgetLinkClicked(const QUrl &)));
				connect(view,SIGNAL(destroyed(QObject *)),SLOT(onStyleWidgetDestroyed(QObject *)));
				emit widgetAdded(view);
			}

			QString html = makeStyleTemplate();
			fillStyleKeywords(html,AOptions);
			view->setHtml(html);

			QTextCursor cursor(view->document());
			cursor.movePosition(QTextCursor::End);
			wstatus.contentStartPosition = cursor.position();
		}
		else
		{
			FWidgetStatus[view].lastKind = -1;
		}

		setVariant(view, AOptions.extended.value(MSO_VARIANT).toString());

		int fontSize = AOptions.extended.value(MSO_FONT_SIZE).toInt();
		QString fontFamily = AOptions.extended.value(MSO_FONT_FAMILY).toString();

		QFont font = view->document()->defaultFont();
		if (fontSize > 0)
			font.setPointSize(fontSize);
		if (!fontFamily.isEmpty())
			font.setFamily(fontFamily);
		view->document()->setDefaultFont(font);

		emit optionsChanged(view,AOptions,AClear);
		return true;
	}
	else if (view == NULL)
	{
		REPORT_ERROR("Failed to change simple style options: Invalid style view");
	}
	return false;
}

bool SimpleMessageStyle::appendContent(QWidget *AWidget, const QString &AHtml, const IMessageStyleContentOptions &AOptions)
{
	StyleViewer *view = qobject_cast<StyleViewer *>(AWidget);
	if (view)
	{
		WidgetStatus &wstatus = FWidgetStatus[AWidget];
		bool scrollAtEnd = !AOptions.noScroll && view->verticalScrollBar()->sliderPosition()==view->verticalScrollBar()->maximum();

		QTextCursor cursor(view->document());
		int maxMessages = Options::node(OPV_MESSAGES_MAXMESSAGESINWINDOW).value().toInt();
		if (maxMessages>0 && wstatus.content.count()>maxMessages+10)
		{
			int scrollMax = view->verticalScrollBar()->maximum();

			int removeSize = 0;
			while(wstatus.content.count() > maxMessages)
				removeSize += wstatus.content.takeFirst().size;

			cursor.setPosition(wstatus.contentStartPosition);
			cursor.setPosition(wstatus.contentStartPosition+removeSize,QTextCursor::KeepAnchor);
			cursor.removeSelectedText();

			if (!scrollAtEnd)
			{
				int newScrollPos = qMax(view->verticalScrollBar()->sliderPosition()-(scrollMax-view->verticalScrollBar()->maximum()),0);
				view->verticalScrollBar()->setSliderPosition(newScrollPos);
			}
		}
		cursor.movePosition(QTextCursor::End);

		QString content = makeContentTemplate(AOptions,wstatus);
		fillContentKeywords(content,AOptions,wstatus);
		content.replace("%message%",prepareMessage(AHtml,AOptions));

		ContentItem item;
		int startPos = cursor.position();
		cursor.insertHtml(content);
		item.size = cursor.position()-startPos;

		if (scrollAtEnd)
			view->verticalScrollBar()->setSliderPosition(view->verticalScrollBar()->maximum());

		wstatus.lastKind = AOptions.kind;
		wstatus.lastId = AOptions.senderId;
		wstatus.lastTime = AOptions.time;
		wstatus.content.append(item);

		emit contentAppended(AWidget,AHtml,AOptions);
		return true;
	}
	else
	{
		REPORT_ERROR("Failed to simple style append content: Invalid view");
	}
	return false;
}

QMap<QString, QVariant> SimpleMessageStyle::infoValues() const
{
	return FInfo;
}

QList<QString> SimpleMessageStyle::variants() const
{
	return FVariants;
}

QList<QString> SimpleMessageStyle::styleVariants(const QString &AStylePath)
{
	QList<QString> files;
	if (!AStylePath.isEmpty())
	{
		QDir dir(AStylePath+"/Variants");
		files = dir.entryList(QStringList("*.css"),QDir::Files,QDir::Name);
		for (int i=0; i<files.count();i++)
			files[i].chop(4);
	}
	else
	{
		REPORT_ERROR("Failed to get simple style variants: Style path is empty");
	}
	return files;
}

QMap<QString, QVariant> SimpleMessageStyle::styleInfo(const QString &AStylePath)
{
	QMap<QString, QVariant> info;

	QFile file(AStylePath+"/Info.plist");
	if (!AStylePath.isEmpty() && file.open(QFile::ReadOnly))
	{
		QString xmlError;
		QDomDocument doc;
		if (doc.setContent(&file,true,&xmlError))
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
		else
		{
			LOG_ERROR(QString("Failed to load simple style info from file content: %1").arg(xmlError));
		}
	}
	else if (!AStylePath.isEmpty())
	{
		LOG_ERROR(QString("Failed to load simple style info from file: %1").arg(file.errorString()));
	}
	else
	{
		REPORT_ERROR("Failed to get simple style info: Style path is empty");
	}
	return info;
}

void SimpleMessageStyle::initStyleSettings()
{
	FCombineConsecutive = !FInfo.value(MSIV_DISABLE_COMBINE_CONSECUTIVE,false).toBool();
	FAllowCustomBackground = !FInfo.value(MSIV_DISABLE_CUSTOM_BACKGROUND,false).toBool();
}

void SimpleMessageStyle::loadTemplates()
{
	FIn_ContentHTML =      loadFileData(FStylePath+"/Incoming/Content.html",QString::null);
	FIn_NextContentHTML =  loadFileData(FStylePath+"/Incoming/NextContent.html",FIn_ContentHTML);

	FOut_ContentHTML =     loadFileData(FStylePath+"/Outgoing/Content.html",FIn_ContentHTML);
	FOut_NextContentHTML = loadFileData(FStylePath+"/Outgoing/NextContent.html",FOut_ContentHTML);

	FTopicHTML =           loadFileData(FStylePath+"/Topic.html",QString::null);
	FStatusHTML =          loadFileData(FStylePath+"/Status.html",FIn_ContentHTML);
	FMeCommandHTML =       loadFileData(FStylePath+"/MeCommand.html",QString::null);
}

void SimpleMessageStyle::loadSenderColors()
{
	QFile colors(FStylePath+"/Incoming/SenderColors.txt");
	if (colors.open(QFile::ReadOnly))
		FSenderColors = QString::fromUtf8(colors.readAll()).split(':',QString::SkipEmptyParts);
}

QString SimpleMessageStyle::loadFileData(const QString &AFileName, const QString &DefValue) const
{
	QFile file(AFileName);
	if (file.open(QFile::ReadOnly))
	{
		QByteArray html = file.readAll();
		return QString::fromUtf8(html.data(),html.size());
	}
	else if (file.exists())
	{
		LOG_ERROR(QString("Failed to load simple style data from file=%1: %2").arg(AFileName,file.errorString()));
	}
	return DefValue;
}

QString SimpleMessageStyle::makeStyleTemplate() const
{
	QString htmlFileName = FStylePath+"/Template.html";
	if (!QFile::exists(htmlFileName))
		htmlFileName =FSharedPath+"/Template.html";
	return loadFileData(htmlFileName,QString::null);;
}

void SimpleMessageStyle::fillStyleKeywords(QString &AHtml, const IMessageStyleOptions &AOptions) const
{
	QString background;
	if (FAllowCustomBackground)
	{
		if (!AOptions.extended.value(MSO_BG_IMAGE_FILE).toString().isEmpty())
		{
			background.append("background-image: url('%1'); ");
			background = background.arg(QUrl::fromLocalFile(AOptions.extended.value(MSO_BG_IMAGE_FILE).toString()).toString());
		}
		if (!AOptions.extended.value(MSO_BG_COLOR).toString().isEmpty())
		{
			background.append(QString("background-color: %1; ").arg(AOptions.extended.value(MSO_BG_COLOR).toString()));
		}
	}
	AHtml.replace("%bodyBackground%", background);
}

void SimpleMessageStyle::setVariant(StyleViewer *AView, const QString &AVariant)
{
	QString variantName = !FVariants.contains(AVariant) ? FInfo.value(MSIV_DEFAULT_VARIANT,"main").toString() : AVariant;
	QString variantFile = QString("Variants/%1.css").arg(variantName);
	AView->document()->setDefaultStyleSheet(loadFileData(FStylePath+"/"+variantFile,QString::null));
}

bool SimpleMessageStyle::isConsecutive(const IMessageStyleContentOptions &AOptions, const WidgetStatus &AStatus) const
{
	if (!FCombineConsecutive)
		return false;

	if (AOptions.kind != IMessageStyleContentOptions::KindMessage)
		return false;
	if (AOptions.senderId.isEmpty())
		return false;

	if (AStatus.lastKind != AOptions.kind)
		return false;
	if (AStatus.lastId != AOptions.senderId)
		return false;
	if (AStatus.lastTime.secsTo(AOptions.time) > CONSECUTIVE_TIMEOUT)
		return false;

	return true;
}

QString SimpleMessageStyle::makeContentTemplate(const IMessageStyleContentOptions &AOptions, const WidgetStatus &AStatus) const
{
	QString html;
	if (AOptions.kind==IMessageStyleContentOptions::KindTopic && !FTopicHTML.isEmpty())
	{
		html = FTopicHTML;
	}
	else if (AOptions.kind==IMessageStyleContentOptions::KindStatus && !FStatusHTML.isEmpty())
	{
		html = FStatusHTML;
	}
	else if (AOptions.kind==IMessageStyleContentOptions::KindMeCommand && (!FMeCommandHTML.isEmpty() || !FStatusHTML.isEmpty()))
	{
		html = !FMeCommandHTML.isEmpty() ? FMeCommandHTML : FStatusHTML;
	}
	else
	{
		bool consecutive = isConsecutive(AOptions,AStatus);
		if (AOptions.direction == IMessageStyleContentOptions::DirectionIn)
			html = consecutive ? FIn_NextContentHTML : FIn_ContentHTML;
		else
			html = consecutive ? FOut_NextContentHTML : FOut_ContentHTML;
	}
	return html;
}

void SimpleMessageStyle::fillContentKeywords(QString &AHtml, const IMessageStyleContentOptions &AOptions, const WidgetStatus &AStatus) const
{
	bool isDirectionIn = AOptions.direction == IMessageStyleContentOptions::DirectionIn;

	QStringList messageClasses;
	if (isConsecutive(AOptions,AStatus))
		messageClasses << MSMC_CONSECUTIVE;

	if (AOptions.kind==IMessageStyleContentOptions::KindMeCommand)
		messageClasses << (!FMeCommandHTML.isEmpty() ? MSMC_MECOMMAND : MSMC_STATUS);
	else if (AOptions.kind == IMessageStyleContentOptions::KindStatus)
		messageClasses << MSMC_STATUS;
	else
		messageClasses << MSMC_MESSAGE;
	
	if (isDirectionIn)
		messageClasses << MSMC_INCOMING;
	else
		messageClasses << MSMC_OUTGOING;
	
	if (AOptions.type & IMessageStyleContentOptions::TypeGroupchat)
		messageClasses << MSMC_GROUPCHAT;
	if (AOptions.type & IMessageStyleContentOptions::TypeHistory)
		messageClasses << MSMC_HISTORY;
	if (AOptions.type & IMessageStyleContentOptions::TypeEvent)
		messageClasses << MSMC_EVENT;
	if (AOptions.type & IMessageStyleContentOptions::TypeMention)
		messageClasses << MSMC_MENTION;
	if (AOptions.type & IMessageStyleContentOptions::TypeNotification)
		messageClasses << MSMC_NOTIFICATION;

	QString messageStatus;
	if (AOptions.status == IMessageStyleContentOptions::StatusOnline)
		messageStatus = MSSK_ONLINE;
	else if (AOptions.status == IMessageStyleContentOptions::StatusOffline)
		messageStatus = MSSK_OFFLINE;
	else if (AOptions.status == IMessageStyleContentOptions::StatusAway)
		messageStatus = MSSK_AWAY;
	else if (AOptions.status == IMessageStyleContentOptions::StatusAwayMessage)
		messageStatus = MSSK_AWAY_MESSAGE;
	else if (AOptions.status == IMessageStyleContentOptions::StatusReturnAway)
		messageStatus = MSSK_RETURN_AWAY;
	else if (AOptions.status == IMessageStyleContentOptions::StatusIdle)
		messageStatus = MSSK_IDLE;
	else if (AOptions.status == IMessageStyleContentOptions::StatusReturnIdle)
		messageStatus = MSSK_RETURN_IDLE;
	else if (AOptions.status == IMessageStyleContentOptions::StatusDateSeparator)
		messageStatus = MSSK_DATE_SEPARATOR;
	else if (AOptions.status == IMessageStyleContentOptions::StatusJoined)
		messageStatus = MSSK_CONTACT_JOINED;
	else if (AOptions.status == IMessageStyleContentOptions::StatusLeft)
		messageStatus = MSSK_CONTACT_LEFT;
	else if (AOptions.status == IMessageStyleContentOptions::StatusError)
		messageStatus = MSSK_ERROR;
	else if (AOptions.status == IMessageStyleContentOptions::StatusTimeout)
		messageStatus = MSSK_TIMED_OUT;
	else if (AOptions.status == IMessageStyleContentOptions::StatusEncryption)
		messageStatus = MSSK_ENCRYPTION;
	else if (AOptions.status == IMessageStyleContentOptions::StatusFileTransferBegan)
		messageStatus = MSSK_FILETRANSFER_BEGAN;
	else if (AOptions.status == IMessageStyleContentOptions::StatusFileTransferComplete)
		messageStatus = MSSK_FILETRANSFER_COMPLETE;
	if (!messageStatus.isEmpty())
		messageClasses << messageStatus;

	AHtml.replace("%messageClasses%", messageClasses.join(" "));

	AHtml.replace("%senderStatusIcon%",AOptions.senderIcon);
	AHtml.replace("%shortTime%", AOptions.time.toString(tr("hh:mm")).toHtmlEscaped());

	QString avatar = AOptions.senderAvatar;
	if (!QFile::exists(avatar))
	{
		avatar = FStylePath+(isDirectionIn ? "/Incoming/buddy_icon.png" : "/Outgoing/buddy_icon.png");
		if (!isDirectionIn && !QFile::exists(avatar))
			avatar = FStylePath+"/Incoming/buddy_icon.png";
		if (!QFile::exists(avatar))
			avatar = FSharedPath+"/buddy_icon.png";
	}
	AHtml.replace("%userIconPath%",avatar);

	QString timeFormat = !AOptions.timeFormat.isEmpty() ? AOptions.timeFormat : tr("hh:mm:ss");
	QString time = AOptions.time.toString(timeFormat).toHtmlEscaped();
	AHtml.replace("%time%", time);

	QString senderColor = AOptions.senderColor;
	if (senderColor.isEmpty())
	{
		if (isDirectionIn)
			senderColor = AStatus.options.value(MSO_CONTACT_COLOR).toString();
		else
			senderColor = AStatus.options.value(MSO_SELF_COLOR).toString();
	}
	AHtml.replace("%senderColor%",senderColor);

	AHtml.replace("%sender%",AOptions.senderName);
	AHtml.replace("%senderScreenName%", QString::null);
	AHtml.replace("%textbackgroundcolor%",!AOptions.textBGColor.isEmpty() ? AOptions.textBGColor : "inherit");
}

QString SimpleMessageStyle::prepareMessage(const QString &AHtml, const IMessageStyleContentOptions &AOptions) const
{
	if (AOptions.kind==IMessageStyleContentOptions::KindMeCommand && FMeCommandHTML.isEmpty())
	{
		QTextDocument doc;
		doc.setHtml(AHtml);
		QTextCursor cursor(&doc);
		cursor.insertHtml(QString("<i>*&nbsp;%1</i>&nbsp;").arg(AOptions.senderName));
		return TextManager::getDocumentBody(doc);
	}
	return AHtml;
}

bool SimpleMessageStyle::eventFilter(QObject *AWatched, QEvent *AEvent)
{
	if (AEvent->type() == QEvent::Resize)
	{
		StyleViewer *view = qobject_cast<StyleViewer *>(AWatched);
		if (FWidgetStatus.contains(view))
		{
			WidgetStatus &wstatus = FWidgetStatus[view];
			if (!wstatus.scrollStarted && view->verticalScrollBar()->sliderPosition()==view->verticalScrollBar()->maximum())
			{
				wstatus.scrollStarted = true;
				FScrollTimer.start();
			}
		}
	}
	return QObject::eventFilter(AWatched,AEvent);
}

void SimpleMessageStyle::onScrollAfterResize()
{
	for (QMap<QWidget*,WidgetStatus>::iterator it = FWidgetStatus.begin(); it!= FWidgetStatus.end(); ++it)
	{
		if (it->scrollStarted)
		{
			StyleViewer *view = qobject_cast<StyleViewer *>(it.key());
			QScrollBar *scrollBar = view->verticalScrollBar();
			scrollBar->setSliderPosition(scrollBar->maximum());
			it->scrollStarted = false;
		}
	}
}

void SimpleMessageStyle::onStyleWidgetLinkClicked(const QUrl &AUrl)
{
	StyleViewer *view = qobject_cast<StyleViewer *>(sender());
	emit urlClicked(view,AUrl);
}

void SimpleMessageStyle::onStyleWidgetDestroyed(QObject *AObject)
{
	FWidgetStatus.remove((QWidget *)AObject);
	emit widgetRemoved((QWidget *)AObject);
}

void SimpleMessageStyle::onStyleWidgetAdded(IMessageStyle *AStyle, QWidget *AWidget)
{
	if (AStyle!=this && FWidgetStatus.contains(AWidget))
	{
		AWidget->removeEventFilter(this);
		FWidgetStatus.remove(AWidget);
		emit widgetRemoved(AWidget);
	}
}
