#include "emoticons.h"

#include <QSet>
#include <QChar>
#include <QMimeData>
#include <QTextBlock>

#define DEFAULT_ICONSET                 "kolobok_dark"

Emoticons::Emoticons()
{
	FMessageWidgets = NULL;
	FMessageProcessor = NULL;
	FOptionsManager = NULL;
}

Emoticons::~Emoticons()
{
	clearTreeItem(&FRootTreeItem);
}

void Emoticons::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Emoticons");
	APluginInfo->description = tr("Allows to use your smiley images in messages");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://www.vacuum-im.org";
	APluginInfo->dependences.append(MESSAGEWIDGETS_UUID);
}

bool Emoticons::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);
	IPlugin *plugin = APluginManager->pluginInterface("IMessageProcessor").value(0,NULL);
	if (plugin)
	{
		FMessageProcessor = qobject_cast<IMessageProcessor *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IMessageWidgets").value(0,NULL);
	if (plugin)
	{
		FMessageWidgets = qobject_cast<IMessageWidgets *>(plugin->instance());
		if (FMessageWidgets)
		{
			connect(FMessageWidgets->instance(),SIGNAL(toolBarWidgetCreated(IMessageToolBarWidget *)),SLOT(onToolBarWidgetCreated(IMessageToolBarWidget *)));
		}
	}

	plugin = APluginManager->pluginInterface("IOptionsManager").value(0,NULL);
	if (plugin)
	{
		FOptionsManager = qobject_cast<IOptionsManager *>(plugin->instance());
	}

	connect(Options::instance(),SIGNAL(optionsOpened()),SLOT(onOptionsOpened()));
	connect(Options::instance(),SIGNAL(optionsChanged(const OptionsNode &)),SLOT(onOptionsChanged(const OptionsNode &)));

	return FMessageWidgets!=NULL;
}

bool Emoticons::initObjects()
{
	if (FMessageProcessor)
	{
		FMessageProcessor->insertMessageWriter(MWO_EMOTICONS,this);
	}
	if (FMessageWidgets)
	{
		FMessageWidgets->insertEditContentsHandler(ECHO_EMOTICONS_CONVERT_IMAGE2TEXT,this);
	}
	return true;
}

bool Emoticons::initSettings()
{
	Options::setDefaultValue(OPV_MESSAGES_EMOTICONS,QStringList() << DEFAULT_ICONSET);

	if (FOptionsManager)
	{
		IOptionsDialogNode dnode = { ONO_EMOTICONS, OPN_EMOTICONS, tr("Emoticons"), MNI_EMOTICONS };
		FOptionsManager->insertOptionsDialogNode(dnode);
		FOptionsManager->insertOptionsHolder(this);
	}
	return true;
}

void Emoticons::writeTextToMessage(int AOrder, Message &AMessage, QTextDocument *ADocument, const QString &ALang)
{
	Q_UNUSED(AMessage);
	Q_UNUSED(ALang);
	if (AOrder == MWO_EMOTICONS)
		replaceImageToText(ADocument);
}

void Emoticons::writeMessageToText(int AOrder, Message &AMessage, QTextDocument *ADocument, const QString &ALang)
{
	Q_UNUSED(AMessage);
	Q_UNUSED(ALang);
	if (AOrder == MWO_EMOTICONS)
		replaceTextToImage(ADocument);
}

QMultiMap<int, IOptionsWidget *> Emoticons::optionsWidgets(const QString &ANodeId, QWidget *AParent)
{
	QMultiMap<int, IOptionsWidget *> widgets;
	if (ANodeId == OPN_EMOTICONS)
	{
		widgets.insertMulti(OWO_EMOTICONS, new EmoticonsOptions(this,AParent));
	}
	return widgets;
}

bool Emoticons::editContentsCreate(int AOrder, IMessageEditWidget *AWidget, QMimeData *AData)
{
	Q_UNUSED(AOrder);
	Q_UNUSED(AWidget);
	Q_UNUSED(AData);
	return false;
}

bool Emoticons::editContentsCanInsert(int AOrder, IMessageEditWidget *AWidget, const QMimeData *AData)
{
	Q_UNUSED(AOrder);
	Q_UNUSED(AWidget);
	Q_UNUSED(AData);
	return false;
}

bool Emoticons::editContentsInsert(int AOrder, IMessageEditWidget *AWidget, const QMimeData *AData, QTextDocument *ADocument)
{
	Q_UNUSED(AOrder); Q_UNUSED(AData);
	if (AOrder == ECHO_EMOTICONS_CONVERT_IMAGE2TEXT)
	{
		if (AWidget->isRichTextEnabled())
		{
			QList<QUrl> urlList = FUrlByKey.values();
			QTextBlock block = ADocument->firstBlock();
			while (block.isValid())
			{
				for (QTextBlock::iterator it = block.begin(); !it.atEnd(); ++it)
				{
					QTextFragment fragment = it.fragment();
					if (fragment.charFormat().isImageFormat())
					{
						QUrl url = fragment.charFormat().toImageFormat().name();
						if (AWidget->document()->resource(QTextDocument::ImageResource,url).isNull())
						{
							if (urlList.contains(url))
							{
								AWidget->document()->addResource(QTextDocument::ImageResource,url,QImage(url.toLocalFile()));
								AWidget->document()->markContentsDirty(fragment.position(),fragment.length());
							}
						}
					}
				}
				block = block.next();
			}
		}
		else
		{
			replaceImageToText(ADocument);
		}
	}
	return false;
}

bool Emoticons::editContentsChanged(int AOrder, IMessageEditWidget *AWidget, int &APosition, int &ARemoved, int &AAdded)
{
	Q_UNUSED(AOrder);
	Q_UNUSED(AWidget);
	Q_UNUSED(APosition);
	Q_UNUSED(ARemoved);
	Q_UNUSED(AAdded);
	return false;
}

QList<QString> Emoticons::activeIconsets() const
{
	QList<QString> iconsets = Options::node(OPV_MESSAGES_EMOTICONS).value().toStringList();
	for (QList<QString>::iterator it = iconsets.begin(); it != iconsets.end(); )
	{
		if (!FStorages.contains(*it))
			it = iconsets.erase(it);
		else
			++it;
	}
	return iconsets;
}

QUrl Emoticons::urlByKey(const QString &AKey) const
{
	return FUrlByKey.value(AKey);
}

QString Emoticons::keyByUrl(const QUrl &AUrl) const
{
	return FKeyByUrl.value(AUrl.toString());
}

QMap<int, QString> Emoticons::findTextEmoticons(const QTextDocument *ADocument, int AStartPos, int ALength) const
{
	QMap<int,QString> emoticons;
	QTextBlock block = ADocument->findBlock(AStartPos);
	int stopPos = ALength < 0 ? ADocument->characterCount() : AStartPos+ALength;
	while (block.isValid() && block.position()<stopPos)
	{
		for (QTextBlock::iterator it = block.begin(); !it.atEnd(); ++it)
		{
			QTextFragment fragment = it.fragment();
			if (fragment.length()>0 && fragment.position()<stopPos)
			{
				bool searchStarted = true;
				QString searchText = fragment.text();
				for (int keyPos=0; keyPos<searchText.length(); keyPos++)
				{
					searchStarted = searchStarted || searchText.at(keyPos).isSpace();
					if (searchStarted && !searchText.at(keyPos).isSpace())
					{
						int keyLength = 0;
						const EmoticonTreeItem *item = &FRootTreeItem;
						while (item && keyLength<=searchText.length()-keyPos && fragment.position()+keyPos+keyLength<=stopPos)
						{
							const QChar nextChar = keyPos+keyLength<searchText.length() ? searchText.at(keyPos+keyLength) : QChar(' ');
							if (!item->url.isEmpty() && nextChar.isSpace())
							{
								emoticons.insert(fragment.position()+keyPos,searchText.mid(keyPos,keyLength));
								keyPos += keyLength-1;
								item = NULL;
							}
							else
							{
								keyLength++;
								item = item->childs.value(nextChar);
							}
						}
						searchStarted = false;
					}
				}
			}
		}
		block = block.next();
	}
	return emoticons;
}

QMap<int, QString> Emoticons::findImageEmoticons(const QTextDocument *ADocument, int AStartPos, int ALength) const
{
	QMap<int,QString> emoticons;
	QTextBlock block = ADocument->findBlock(AStartPos);
	int stopPos = ALength < 0 ? ADocument->characterCount() : AStartPos+ALength;
	while (block.isValid() && block.position()<stopPos)
	{
		for (QTextBlock::iterator it = block.begin(); !it.atEnd() && it.fragment().position()<stopPos; ++it)
		{
			if (it.fragment().charFormat().isImageFormat())
			{
				QString key = FKeyByUrl.value(it.fragment().charFormat().toImageFormat().name());
				if (!key.isEmpty() && it.fragment().length()==1)
					emoticons.insert(it.fragment().position(),key);
			}
		}
		block = block.next();
	}
	return emoticons;
}

void Emoticons::createIconsetUrls()
{
	FUrlByKey.clear();
	FKeyByUrl.clear();
	clearTreeItem(&FRootTreeItem);
	foreach(QString substorage, Options::node(OPV_MESSAGES_EMOTICONS).value().toStringList())
	{
		IconStorage *storage = FStorages.value(substorage);
		if (storage)
		{
			QHash<QString, QString> fileFirstKey;
			foreach(QString key, storage->fileFirstKeys())
				fileFirstKey.insert(storage->fileFullName(key), key);

			foreach(QString key, storage->fileKeys())
			{
				if (!FUrlByKey.contains(key)) 
				{
					QString file = storage->fileFullName(key);
					QUrl url = QUrl::fromLocalFile(file);
					FUrlByKey.insert(key,url);
					FKeyByUrl.insert(url.toString(),fileFirstKey.value(file));
					createTreeItem(key,url);
				}
			}
		}
	}
}

void Emoticons::createTreeItem(const QString &AKey, const QUrl &AUrl)
{
	EmoticonTreeItem *item = &FRootTreeItem;
	for (int i=0; i<AKey.size(); i++)
	{
		QChar itemChar = AKey.at(i);
		if (!item->childs.contains(itemChar))
		{
			EmoticonTreeItem *childItem = new EmoticonTreeItem;
			item->childs.insert(itemChar,childItem);
			item = childItem;
		}
		else
		{
			item = item->childs.value(itemChar);
		}
	}
	item->url = AUrl;
}

void Emoticons::clearTreeItem(EmoticonTreeItem *AItem) const
{
	foreach(QChar itemChar, AItem->childs.keys())
	{
		EmoticonTreeItem *childItem = AItem->childs.take(itemChar);
		clearTreeItem(childItem);
		delete childItem;
	}
}

bool Emoticons::isWordBoundary(const QString &AText) const
{
	return !AText.isEmpty() ? AText.at(0).isSpace() : true;
}

int Emoticons::replaceTextToImage(QTextDocument *ADocument, int AStartPos, int ALength) const
{
	int posOffset = 0;
	QMap<int,QString> emoticons = findTextEmoticons(ADocument,AStartPos,ALength);
	if (!emoticons.isEmpty())
	{
		QTextCursor cursor(ADocument);
		cursor.beginEditBlock();
		for (QMap<int,QString>::const_iterator it=emoticons.constBegin(); it!=emoticons.constEnd(); ++it)
		{
			QUrl url = FUrlByKey.value(it.value());
			if (!url.isEmpty())
			{
				cursor.setPosition(it.key()-posOffset);
				cursor.movePosition(QTextCursor::NextCharacter,QTextCursor::KeepAnchor,it->length());
				if (!ADocument->resource(QTextDocument::ImageResource,url).isValid())
					cursor.insertImage(QImage(url.toLocalFile()),url.toString());
				else
					cursor.insertImage(url.toString());
				posOffset += it->length()-1;
			}
		}
		cursor.endEditBlock();
	}
	return posOffset;
}

int Emoticons::replaceImageToText(QTextDocument *ADocument, int AStartPos, int ALength) const
{
	int posOffset = 0;
	QMap<int,QString> emoticons = findImageEmoticons(ADocument,AStartPos,ALength);
	if (!emoticons.isEmpty())
	{
		QTextCursor cursor(ADocument);
		cursor.beginEditBlock();
		for (QMap<int,QString>::const_iterator it=emoticons.constBegin(); it!=emoticons.constEnd(); ++it)
		{
			cursor.setPosition(it.key()+posOffset);
			cursor.deleteChar();
			posOffset--;

			if (cursor.movePosition(QTextCursor::PreviousCharacter,QTextCursor::KeepAnchor,1))
			{
				bool space = !isWordBoundary(cursor.selectedText());
				cursor.movePosition(QTextCursor::NextCharacter,QTextCursor::MoveAnchor,1);
				if (space)
				{
					posOffset++;
					cursor.insertText(" ");
				}
			}

			cursor.insertText(it.value());
			posOffset += it->length();

			if (cursor.movePosition(QTextCursor::NextCharacter,QTextCursor::KeepAnchor,1))
			{
				bool space = !isWordBoundary(cursor.selectedText());
				cursor.movePosition(QTextCursor::PreviousCharacter,QTextCursor::MoveAnchor,1);
				if (space)
				{
					posOffset++;
					cursor.insertText(" ");
				}
			}

		}
		cursor.endEditBlock();
	}
	return posOffset;
}

SelectIconMenu *Emoticons::createSelectIconMenu(const QString &ASubStorage, QWidget *AParent)
{
	SelectIconMenu *menu = new SelectIconMenu(ASubStorage, AParent);
	connect(menu->instance(),SIGNAL(iconSelected(const QString &, const QString &)), SLOT(onIconSelected(const QString &, const QString &)));
	connect(menu->instance(),SIGNAL(destroyed(QObject *)),SLOT(onSelectIconMenuDestroyed(QObject *)));
	return menu;
}

void Emoticons::insertSelectIconMenu(const QString &ASubStorage)
{
	foreach(IMessageToolBarWidget *widget, FToolBarsWidgets)
	{
		SelectIconMenu *menu = createSelectIconMenu(ASubStorage,widget->instance());
		FToolBarWidgetByMenu.insert(menu,widget);
		QToolButton *button = widget->toolBarChanger()->insertAction(menu->menuAction(),TBG_MWTBW_EMOTICONS);
		button->setToolButtonStyle(Qt::ToolButtonIconOnly);
		button->setPopupMode(QToolButton::InstantPopup);
	}
}

void Emoticons::removeSelectIconMenu(const QString &ASubStorage)
{
	QMap<SelectIconMenu *,IMessageToolBarWidget *>::iterator it = FToolBarWidgetByMenu.begin();
	while (it != FToolBarWidgetByMenu.end())
	{
		SelectIconMenu *menu = it.key();
		IMessageToolBarWidget *widget = it.value();
		if (menu->iconset() == ASubStorage)
		{
			widget->toolBarChanger()->removeItem(widget->toolBarChanger()->actionHandle(menu->menuAction()));
			it = FToolBarWidgetByMenu.erase(it);
			delete menu;
		}
		else
			++it;
	}
}

void Emoticons::onToolBarWidgetCreated(IMessageToolBarWidget *AWidget)
{
	if (AWidget->messageWindow()->editWidget() != NULL)
	{
		FToolBarsWidgets.append(AWidget);
		foreach(QString substorage, activeIconsets())
		{
			SelectIconMenu *menu = createSelectIconMenu(substorage,AWidget->instance());
			FToolBarWidgetByMenu.insert(menu,AWidget);
			QToolButton *button = AWidget->toolBarChanger()->insertAction(menu->menuAction(),TBG_MWTBW_EMOTICONS);
			button->setToolButtonStyle(Qt::ToolButtonIconOnly);
			button->setPopupMode(QToolButton::InstantPopup);
		}
		connect(AWidget->instance(),SIGNAL(destroyed(QObject *)),SLOT(onToolBarWidgetDestroyed(QObject *)));
	}
}

void Emoticons::onToolBarWidgetDestroyed(QObject *AObject)
{
	QList<IMessageToolBarWidget *>::iterator it = FToolBarsWidgets.begin();
	while (it != FToolBarsWidgets.end())
	{
		if (qobject_cast<QObject *>((*it)->instance()) == AObject)
			it = FToolBarsWidgets.erase(it);
		else
			++it;
	}
}

void Emoticons::onIconSelected(const QString &ASubStorage, const QString &AIconKey)
{
	Q_UNUSED(ASubStorage);
	SelectIconMenu *menu = qobject_cast<SelectIconMenu *>(sender());
	if (FToolBarWidgetByMenu.contains(menu))
	{
		IMessageEditWidget *widget = FToolBarWidgetByMenu.value(menu)->messageWindow()->editWidget();
		if (widget)
		{
			QUrl url = FUrlByKey.value(AIconKey);
			if (!url.isEmpty())
			{
				QTextEdit *editor = widget->textEdit();
				QTextCursor cursor = editor->textCursor();
				cursor.beginEditBlock();

				if (cursor.movePosition(QTextCursor::PreviousCharacter,QTextCursor::KeepAnchor,1))
				{
					bool space = !isWordBoundary(cursor.selectedText());
					cursor.movePosition(QTextCursor::NextCharacter,QTextCursor::MoveAnchor,1);
					if (space)
						cursor.insertText(" ");
				}
				
				if (widget->isRichTextEnabled())
				{
					if (!editor->document()->resource(QTextDocument::ImageResource,url).isValid())
						editor->document()->addResource(QTextDocument::ImageResource,url,QImage(url.toLocalFile()));
					cursor.insertImage(url.toString());
				}
				else
				{
					cursor.insertText(AIconKey);
				}

				if (cursor.movePosition(QTextCursor::NextCharacter,QTextCursor::KeepAnchor,1))
				{
					bool space = !isWordBoundary(cursor.selectedText());
					cursor.movePosition(QTextCursor::PreviousCharacter,QTextCursor::MoveAnchor,1);
					if (space)
						cursor.insertText(" ");
				}

				cursor.endEditBlock();
				editor->setFocus();
			}
		}
	}
}

void Emoticons::onSelectIconMenuDestroyed(QObject *AObject)
{
	foreach(SelectIconMenu *menu, FToolBarWidgetByMenu.keys())
		if (qobject_cast<QObject *>(menu) == AObject)
			FToolBarWidgetByMenu.remove(menu);
}

void Emoticons::onOptionsOpened()
{
	onOptionsChanged(Options::node(OPV_MESSAGES_EMOTICONS));
}

void Emoticons::onOptionsChanged(const OptionsNode &ANode)
{
	if (ANode.path() == OPV_MESSAGES_EMOTICONS)
	{
		QList<QString> oldStorages = FStorages.keys();
		QList<QString> availStorages = IconStorage::availSubStorages(RSR_STORAGE_EMOTICONS);

		foreach(QString substorage, Options::node(OPV_MESSAGES_EMOTICONS).value().toStringList())
		{
			if (availStorages.contains(substorage))
			{
				if (!FStorages.contains(substorage))
				{
					FStorages.insert(substorage, new IconStorage(RSR_STORAGE_EMOTICONS,substorage,this));
					insertSelectIconMenu(substorage);
				}
				oldStorages.removeAll(substorage);
			}
		}

		foreach (QString substorage, oldStorages)
		{
			removeSelectIconMenu(substorage);
			delete FStorages.take(substorage);
		}

		createIconsetUrls();
	}
}

Q_EXPORT_PLUGIN2(plg_emoticons, Emoticons)
