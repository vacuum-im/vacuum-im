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

	return QSortFilterProxyModel::lessThan(ALeft,ARight);
}

ShortcutOptionsWidget::ShortcutOptionsWidget(QWidget *AParent) : QWidget(AParent)
{
	ui.setupUi(this);
	createTreeModel();

	FSortModel.setSourceModel(&FModel);
	FSortModel.setSortLocaleAware(true);
	FSortModel.setSortCaseSensitivity(Qt::CaseInsensitive);

	ui.trvShortcuts->setItemDelegate(new ShortcutOptionsDelegate(ui.trvShortcuts));
	ui.trvShortcuts->setModel(&FSortModel);
	ui.trvShortcuts->header()->setSortIndicatorShown(false);
	ui.trvShortcuts->header()->setResizeMode(COL_NAME,QHeaderView::Stretch);
	ui.trvShortcuts->header()->setResizeMode(COL_KEY,QHeaderView::ResizeToContents);
	ui.trvShortcuts->sortByColumn(COL_NAME,Qt::AscendingOrder);
	ui.trvShortcuts->expandAll();

	connect(ui.pbtDefault,SIGNAL(clicked()),SLOT(onDefaultClicked()));
	connect(ui.pbtClear,SIGNAL(clicked()),SLOT(onClearClicked()));
	connect(ui.pbtRestoreDefaults,SIGNAL(clicked()),SLOT(onRestoreDefaultsClicked()));
	connect(&FModel,SIGNAL(itemChanged(QStandardItem *)),SLOT(onModelItemChanged(QStandardItem *)));
	connect(ui.trvShortcuts,SIGNAL(doubleClicked(const QModelIndex &)),SLOT(onIndexDoubleClicked(const QModelIndex &)));

	reset();
}

ShortcutOptionsWidget::~ShortcutOptionsWidget()
{

}

void ShortcutOptionsWidget::apply()
{
	foreach(QString shortcut, Shortcuts::shortcuts())
	{
		QStandardItem *action = FShortcutItem.value(shortcut);
		if (action)
		{
			Shortcuts::Descriptor descriptor = Shortcuts::shortcutDescriptor(shortcut);
			QStandardItem *key = action->parent()->child(action->row(),COL_KEY);
			QKeySequence activeKey = qvariant_cast<QKeySequence>(key->data(MDR_ACTIVE_KEYSEQUENCE));
			if (descriptor.activeKey != activeKey)
			{
				Shortcuts::updateShortcut(shortcut, activeKey);
				onModelItemChanged(action);
			}
		}
	}
	emit childApply();
}

void ShortcutOptionsWidget::reset()
{
	foreach(QString shortcut, Shortcuts::shortcuts())
	{
		QStandardItem *action = FShortcutItem.value(shortcut);
		if (action)
		{
			Shortcuts::Descriptor descriptor = Shortcuts::shortcutDescriptor(shortcut);
			QStandardItem *key = action->parent()->child(action->row(),COL_KEY);
			key->setText(descriptor.activeKey.toString(QKeySequence::NativeText));
			key->setData(descriptor.activeKey,MDR_ACTIVE_KEYSEQUENCE);
		}
	}
	emit childReset();
}

void ShortcutOptionsWidget::createTreeModel()
{
	FModel.clear();
	FModel.setColumnCount(2);
	FModel.setHorizontalHeaderLabels(QStringList() << tr("Action") << tr("Shortcut"));

	foreach(QString shortcut, Shortcuts::shortcuts())
	{
		Shortcuts::Descriptor descriptor = Shortcuts::shortcutDescriptor(shortcut);
		if (!descriptor.description.isEmpty())
		{
			QStandardItem *action = createTreeRow(shortcut,FModel.invisibleRootItem(),false);
			action->setText(descriptor.description);

			QStandardItem *key = action->parent()->child(action->row(),COL_KEY);
			key->setFlags(Qt::ItemIsEnabled|Qt::ItemIsSelectable|Qt::ItemIsEditable);
			key->setData(shortcut,MDR_SHORTCUTID);
			key->setData(descriptor.defaultKey,MDR_DEFAULT_KEYSEQUENCE);
		}
	}
}

QStandardItem *ShortcutOptionsWidget::createTreeRow(const QString &AId, QStandardItem *AParent, bool AGroup)
{
	QStandardItem *itemAction = FShortcutItem.value(AId);
	if (itemAction == NULL)
	{
		int dotIndex = AId.lastIndexOf('.');
		QString actionName = dotIndex>0 ? AId.mid(dotIndex+1) : AId;
		QString actionPath = dotIndex>0 ? AId.left(dotIndex) : QString::null;
		
		QString actionText = AGroup ? Shortcuts::groupDescription(AId) : QString::null;
		itemAction = new QStandardItem(!actionText.isEmpty() ? actionText : actionName);
		itemAction->setFlags(Qt::ItemIsEnabled|Qt::ItemIsSelectable);

		QStandardItem *itemKey = new QStandardItem;
		itemKey->setFlags(Qt::ItemIsEnabled|Qt::ItemIsSelectable);

		QStandardItem *parentItem = !actionPath.isEmpty() ? createTreeRow(actionPath,AParent,true) : AParent;
		parentItem->appendRow(QList<QStandardItem *>() << itemAction << itemKey);

		FShortcutItem.insert(AId,itemAction);
	}
	return itemAction;
}

void ShortcutOptionsWidget::setItemRed(QStandardItem *AItem, bool ARed) const
{
	AItem->setForeground(ARed ? Qt::red : QBrush()/*this->palette().color(QPalette::WindowText)*/);
}

void ShortcutOptionsWidget::setItemBold(QStandardItem *AItem, bool ABold) const
{
	QFont font = AItem->font();
	font.setBold(ABold);
	AItem->setFont(font);
}

bool ShortcutOptionsWidget::isGlobalKeyFailed(const QString &AId, const QKeySequence &ANewKey) const
{
	if (Shortcuts::globalShortcuts().contains(AId) && !ANewKey.isEmpty())
	{
		return !Shortcuts::isGlobalShortcutActive(AId) && Shortcuts::shortcutDescriptor(AId).activeKey==ANewKey;
	}
	return false;
}

void ShortcutOptionsWidget::onDefaultClicked()
{
	QStandardItem *item = FModel.itemFromIndex(FSortModel.mapToSource(ui.trvShortcuts->currentIndex()));
	QStandardItem *action = item!=NULL && item->parent()!=NULL ? item->parent()->child(item->row(),COL_NAME) : NULL;
	QString shortcut = FShortcutItem.key(action);
	if (Shortcuts::shortcuts().contains(shortcut))
	{
		Shortcuts::Descriptor descriptor = Shortcuts::shortcutDescriptor(shortcut);
		QStandardItem *key = action->parent()->child(action->row(),COL_KEY);
		key->setText(descriptor.defaultKey.toString(QKeySequence::NativeText));
		key->setData(descriptor.defaultKey,MDR_ACTIVE_KEYSEQUENCE);
	}
	ui.trvShortcuts->setFocus();
}

void ShortcutOptionsWidget::onClearClicked()
{
	QStandardItem *item = FModel.itemFromIndex(FSortModel.mapToSource(ui.trvShortcuts->currentIndex()));
	QStandardItem *action = item!=NULL && item->parent()!=NULL ? item->parent()->child(item->row(),COL_NAME) : NULL;
	QString shortcut = FShortcutItem.key(action);
	if (Shortcuts::shortcuts().contains(shortcut))
	{
		QStandardItem *key = action->parent()->child(action->row(),COL_KEY);
		key->setText(QString::null);
		key->setData(QKeySequence(QKeySequence::UnknownKey),MDR_ACTIVE_KEYSEQUENCE);
	}
	ui.trvShortcuts->setFocus();
}

void ShortcutOptionsWidget::onRestoreDefaultsClicked()
{
	foreach(QString shortcut, Shortcuts::shortcuts())
	{
		QStandardItem *action = FShortcutItem.value(shortcut);
		if (action)
		{
			Shortcuts::Descriptor descriptor = Shortcuts::shortcutDescriptor(shortcut);
			QStandardItem *key = action->parent()->child(action->row(),COL_KEY);
			key->setText(descriptor.defaultKey.toString(QKeySequence::NativeText));
			key->setData(descriptor.defaultKey,MDR_ACTIVE_KEYSEQUENCE);
		}
	}
	ui.trvShortcuts->setFocus();
}

void ShortcutOptionsWidget::onModelItemChanged(QStandardItem *AItem)
{
	static bool blocked = false;
	QStandardItem *action = AItem->parent()!=NULL ? AItem->parent()->child(AItem->row(),COL_NAME) : NULL;
	QStandardItem *key = AItem->parent()!=NULL ? AItem->parent()->child(AItem->row(),COL_KEY) : NULL;
	if (!blocked && action!=NULL && key!=NULL)
	{
		blocked = true;

		QMap<QKeySequence, int> keyCount;
		for (int row=0; row<AItem->parent()->rowCount(); row++)
		{
			QKeySequence childKey = AItem->parent()->child(row,COL_KEY)->data(MDR_ACTIVE_KEYSEQUENCE).toString();
			keyCount[childKey]++;
		}
		for (int row=0; row<AItem->parent()->rowCount(); row++)
		{
			QKeySequence childKey = AItem->parent()->child(row,COL_KEY)->data(MDR_ACTIVE_KEYSEQUENCE).toString();
			bool conflict = !childKey.isEmpty() && keyCount.value(childKey)>1;
			setItemRed(AItem->parent()->child(row,COL_NAME),conflict);
			setItemRed(AItem->parent()->child(row,COL_KEY),conflict);
		}
		
		if (isGlobalKeyFailed(key->data(MDR_SHORTCUTID).toString(),key->data(MDR_ACTIVE_KEYSEQUENCE).toString()))
		{
			setItemRed(action,true);
			setItemRed(key,true);
		}

		bool notDefault = key->data(MDR_ACTIVE_KEYSEQUENCE).toString()!=key->data(MDR_DEFAULT_KEYSEQUENCE).toString();
		setItemBold(action, notDefault);
		setItemBold(key, notDefault);

		blocked = false;

		emit modified();
	}
}

void ShortcutOptionsWidget::onIndexDoubleClicked(const QModelIndex &AIndex)
{
	QModelIndex editIndex = AIndex.parent().child(AIndex.row(),1);
	if (editIndex.isValid() && (editIndex.flags() & Qt::ItemIsEditable)>0)
		ui.trvShortcuts->edit(editIndex);
}
