#include "shortcutoptionswidget.h"

#include <QHeaderView>

bool SortFilterProxyModel::lessThan(const QModelIndex &ALeft, const QModelIndex &ARight) const
{
	bool leftHasChild = ALeft.child(0,0).isValid();
	bool rightHasChild = ARight.child(0,0).isValid();
	
	if (leftHasChild && !rightHasChild)
		return true;
	else if (!leftHasChild && rightHasChild)
		return false;
	else if (leftHasChild && rightHasChild)
		return ALeft.data(MDR_SORTROLE).toInt() < ARight.data(MDR_SORTROLE).toInt();

	return QSortFilterProxyModel::lessThan(ALeft,ARight);
}

ShortcutOptionsWidget::ShortcutOptionsWidget(QWidget *AParent) : QWidget(AParent)
{
	ui.setupUi(this);
	createTreeModel();
	onRestoreDefaultsClicked();

	FBlockChangesCheck = 0;

	FSortModel.setSourceModel(&FModel);
	FSortModel.setSortLocaleAware(true);
	FSortModel.setSortCaseSensitivity(Qt::CaseInsensitive);

	ui.trvShortcuts->setItemDelegate(new ShortcutOptionsDelegate(ui.trvShortcuts));
	ui.trvShortcuts->setModel(&FSortModel);
	ui.trvShortcuts->header()->setSortIndicatorShown(false);
	ui.trvShortcuts->header()->setSectionResizeMode(COL_NAME,QHeaderView::Stretch);
	ui.trvShortcuts->header()->setSectionResizeMode(COL_KEY,QHeaderView::ResizeToContents);
	ui.trvShortcuts->sortByColumn(COL_NAME,Qt::AscendingOrder);
	ui.trvShortcuts->expandAll();

	FConflictTimer.setInterval(0);
	FConflictTimer.setSingleShot(true);
	connect(&FConflictTimer,SIGNAL(timeout()),SLOT(onShowConflictsTimerTimeout()));

	connect(ui.pbtDefault,SIGNAL(clicked()),SLOT(onDefaultClicked()));
	connect(ui.pbtClear,SIGNAL(clicked()),SLOT(onClearClicked()));
	connect(ui.pbtRestoreDefaults,SIGNAL(clicked()),SLOT(onRestoreDefaultsClicked()));
	connect(&FModel,SIGNAL(itemChanged(QStandardItem *)),SLOT(onModelItemChanged(QStandardItem *)));
	connect(ui.trvShortcuts,SIGNAL(doubleClicked(const QModelIndex &)),SLOT(onIndexDoubleClicked(const QModelIndex &)));

	reset();
	FConflictTimer.start();
}

ShortcutOptionsWidget::~ShortcutOptionsWidget()
{

}

void ShortcutOptionsWidget::apply()
{
	foreach(const QString &shortcut, Shortcuts::shortcuts())
	{
		QStandardItem *nameItem = FShortcutItem.value(shortcut);
		if (nameItem)
		{
			Shortcuts::Descriptor descriptor = Shortcuts::shortcutDescriptor(shortcut);
			QStandardItem *keyItem = nameItem->parent()->child(nameItem->row(),COL_KEY);
			QKeySequence activeKey = qvariant_cast<QKeySequence>(keyItem->data(MDR_ACTIVE_KEYSEQUENCE));
			if (descriptor.activeKey != activeKey)
			{
				Shortcuts::updateShortcut(shortcut, activeKey);
				FConflictTimer.start();
			}
		}
	}
	emit childApply();
}

void ShortcutOptionsWidget::reset()
{
	foreach(const QString &shortcut, Shortcuts::shortcuts())
	{
		QStandardItem *nameItem = FShortcutItem.value(shortcut);
		if (nameItem)
		{
			Shortcuts::Descriptor descriptor = Shortcuts::shortcutDescriptor(shortcut);
			QStandardItem *keyItem = nameItem->parent()->child(nameItem->row(),COL_KEY);
			keyItem->setText(descriptor.activeKey.toString(QKeySequence::NativeText));
			keyItem->setData(descriptor.activeKey,MDR_ACTIVE_KEYSEQUENCE);
		}
	}
	emit childReset();
}

void ShortcutOptionsWidget::createTreeModel()
{
	FModel.clear();
	FModel.setColumnCount(2);
	FModel.setHorizontalHeaderLabels(QStringList() << tr("Action") << tr("Shortcut"));

	foreach(const QString &shortcut, Shortcuts::shortcuts())
	{
		Shortcuts::Descriptor descriptor = Shortcuts::shortcutDescriptor(shortcut);
		if (!descriptor.description.isEmpty())
		{
			QStandardItem *nameItem = createTreeRow(shortcut,FModel.invisibleRootItem(),false);
			nameItem->setText(descriptor.description);

			QStandardItem *keyItem = nameItem->parent()->child(nameItem->row(),COL_KEY);
			keyItem->setFlags(Qt::ItemIsEnabled|Qt::ItemIsSelectable|Qt::ItemIsEditable);
			keyItem->setData(shortcut,MDR_SHORTCUTID);
			keyItem->setData(descriptor.defaultKey,MDR_DEFAULT_KEYSEQUENCE);
		}
	}

	foreach(const QString &shortcut, Shortcuts::globalShortcuts())
	{
		QStandardItem *nameItem = FShortcutItem.value(shortcut);
		if (nameItem)
			FGlobalItems.append(nameItem);
	}
}

QStandardItem *ShortcutOptionsWidget::createTreeRow(const QString &AId, QStandardItem *AParent, bool AGroup)
{
	QStandardItem *nameItem = FShortcutItem.value(AId);
	if (nameItem == NULL)
	{
		int dotIndex = AId.lastIndexOf('.');
		QString actionName = dotIndex>0 ? AId.mid(dotIndex+1) : AId;
		QString actionPath = dotIndex>0 ? AId.left(dotIndex) : QString::null;
		
		QString actionText = AGroup ? Shortcuts::groupDescription(AId) : QString::null;
		nameItem = new QStandardItem(!actionText.isEmpty() ? actionText : actionName);
		nameItem->setFlags(Qt::ItemIsEnabled|Qt::ItemIsSelectable);
		nameItem->setData(AGroup ? Shortcuts::groupOrder(AId) : 0, MDR_SORTROLE);

		QStandardItem *keyItem = new QStandardItem;
		keyItem->setFlags(Qt::ItemIsEnabled|Qt::ItemIsSelectable);

		QStandardItem *parentItem = !actionPath.isEmpty() ? createTreeRow(actionPath,AParent,true) : AParent;
		parentItem->appendRow(QList<QStandardItem *>() << nameItem << keyItem);

		FShortcutItem.insert(AId,nameItem);
	}
	return nameItem;
}

void ShortcutOptionsWidget::setItemRed(QStandardItem *AItem, bool ARed) const
{
	AItem->setForeground(ARed ? Qt::red : ui.trvShortcuts->palette().color(QPalette::Text));
}

void ShortcutOptionsWidget::setItemBold(QStandardItem *AItem, bool ABold) const
{
	QFont font = AItem->font();
	font.setBold(ABold);
	AItem->setFont(font);
}

void ShortcutOptionsWidget::onDefaultClicked()
{
	QStandardItem *item = FModel.itemFromIndex(FSortModel.mapToSource(ui.trvShortcuts->currentIndex()));
	QStandardItem *nameItem = item!=NULL && item->parent()!=NULL ? item->parent()->child(item->row(),COL_NAME) : NULL;
	QString shortcut = FShortcutItem.key(nameItem);
	if (Shortcuts::shortcuts().contains(shortcut))
	{
		Shortcuts::Descriptor descriptor = Shortcuts::shortcutDescriptor(shortcut);
		QStandardItem *keyItem = nameItem->parent()->child(nameItem->row(),COL_KEY);
		keyItem->setText(descriptor.defaultKey.toString(QKeySequence::NativeText));
		keyItem->setData(descriptor.defaultKey,MDR_ACTIVE_KEYSEQUENCE);
	}
	ui.trvShortcuts->setFocus();
}

void ShortcutOptionsWidget::onClearClicked()
{
	QStandardItem *item = FModel.itemFromIndex(FSortModel.mapToSource(ui.trvShortcuts->currentIndex()));
	QStandardItem *nameItem = item!=NULL && item->parent()!=NULL ? item->parent()->child(item->row(),COL_NAME) : NULL;
	QString shortcut = FShortcutItem.key(nameItem);
	if (Shortcuts::shortcuts().contains(shortcut))
	{
		QStandardItem *keyItem = nameItem->parent()->child(nameItem->row(),COL_KEY);
		keyItem->setText(QString::null);
		keyItem->setData(QKeySequence(QKeySequence::UnknownKey),MDR_ACTIVE_KEYSEQUENCE);
	}
	ui.trvShortcuts->setFocus();
}

void ShortcutOptionsWidget::onRestoreDefaultsClicked()
{
	foreach(const QString &shortcut, Shortcuts::shortcuts())
	{
		QStandardItem *nameItem = FShortcutItem.value(shortcut);
		if (nameItem)
		{
			Shortcuts::Descriptor descriptor = Shortcuts::shortcutDescriptor(shortcut);
			QStandardItem *keyItem = nameItem->parent()->child(nameItem->row(),COL_KEY);
			keyItem->setText(descriptor.defaultKey.toString(QKeySequence::NativeText));
			keyItem->setData(descriptor.defaultKey,MDR_ACTIVE_KEYSEQUENCE);
		}
	}
	ui.trvShortcuts->setFocus();
}

void ShortcutOptionsWidget::onShowConflictsTimerTimeout()
{
	FBlockChangesCheck++;

	for (QMap<QStandardItem *, QKeySequence>::const_iterator it=FItemKeys.constBegin(); it!=FItemKeys.constEnd(); ++it)
	{
		bool conflict = false;

		QList<QStandardItem *> sameItems = FItemKeys.keys(it.value());
		if (!FGlobalItems.contains(it.key()))
		{
			for(int i=0; !conflict && i<FGlobalItems.count(); i++)
				conflict = FItemKeys.value(FGlobalItems.at(i)) == it.value();
		}
		else
		{
			QString shortcut = FShortcutItem.key(it.key());
			conflict = sameItems.count()>1;
			conflict |= !Shortcuts::isGlobalShortcutActive(shortcut) && Shortcuts::shortcutDescriptor(shortcut).activeKey==it.value();
		}
		
		for(int i=0; !conflict && i<sameItems.count(); i++)
			conflict = sameItems.at(i)!=it.key() && sameItems.at(i)->parent()==it.key()->parent();

		setItemRed(it.key(),conflict);
		setItemRed(it.key()->parent()->child(it.key()->row(),COL_KEY),conflict);
	}

	FBlockChangesCheck--;
}

void ShortcutOptionsWidget::onModelItemChanged(QStandardItem *AItem)
{
	QStandardItem *nameItem = AItem->parent()!=NULL ? AItem->parent()->child(AItem->row(),COL_NAME) : NULL;
	QStandardItem *keyItem = AItem->parent()!=NULL ? AItem->parent()->child(AItem->row(),COL_KEY) : NULL;
	if (FBlockChangesCheck<=0 && nameItem!=NULL && keyItem!=NULL)
	{
		FBlockChangesCheck++;

		QKeySequence oldKey = FItemKeys.value(nameItem);
		QKeySequence newKey = keyItem->data(MDR_ACTIVE_KEYSEQUENCE).toString();
		if (oldKey != newKey)
		{
			if (newKey.isEmpty())
			{
				FItemKeys.remove(nameItem);
				setItemRed(nameItem,false);
				setItemRed(keyItem,false);
			}
			else
			{
				FItemKeys.insert(nameItem,newKey);
			}

			bool notDefault = keyItem->data(MDR_ACTIVE_KEYSEQUENCE).toString()!=keyItem->data(MDR_DEFAULT_KEYSEQUENCE).toString();
			setItemBold(nameItem, notDefault);
			setItemBold(keyItem, notDefault);
			
			FConflictTimer.start();
			emit modified();
		}

		FBlockChangesCheck--;
	}
}

void ShortcutOptionsWidget::onIndexDoubleClicked(const QModelIndex &AIndex)
{
	QModelIndex editIndex = AIndex.parent().child(AIndex.row(),1);
	if (editIndex.isValid() && (editIndex.flags() & Qt::ItemIsEditable)>0)
		ui.trvShortcuts->edit(editIndex);
}
