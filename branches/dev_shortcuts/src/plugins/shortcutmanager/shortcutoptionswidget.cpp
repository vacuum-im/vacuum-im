#include "shortcutoptionswidget.h"

#include <QHeaderView>

ShortcutOptionsWidget::ShortcutOptionsWidget(QWidget *AParent) : QWidget(AParent)
{
	ui.setupUi(this);
	createTreeModel();

	FSortModel.setSourceModel(&FModel);
	FSortModel.setSortLocaleAware(true);
	FSortModel.setSortCaseSensitivity(Qt::CaseInsensitive);

	ui.trvShortcuts->setItemDelegate(new ShortcutOptionsDelegate(ui.trvShortcuts));
	ui.trvShortcuts->setModel(&FSortModel);
	ui.trvShortcuts->header()->setResizeMode(COL_NAME,QHeaderView::Stretch);
	ui.trvShortcuts->header()->setResizeMode(COL_KEY,QHeaderView::ResizeToContents);
	ui.trvShortcuts->sortByColumn(COL_NAME,Qt::AscendingOrder);
	ui.trvShortcuts->expandAll();

	reset();

	connect(&FModel,SIGNAL(itemChanged(QStandardItem *)),SIGNAL(modified()));
	connect(ui.pbtResetToDefaults,SIGNAL(clicked()),SLOT(onResetDefaultsClicked()));
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
			ShortcutDescriptor descriptor = Shortcuts::shortcutDescriptor(shortcut);
			QStandardItem *key = action->parent()->child(action->row(),COL_KEY);
			QKeySequence userKey = key->text();
			if (descriptor.userKey != userKey)
				Shortcuts::updateShortcut(shortcut, userKey!=descriptor.defaultKey ? userKey : QKeySequence());
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
			ShortcutDescriptor descriptor = Shortcuts::shortcutDescriptor(shortcut);
			QStandardItem *key = action->parent()->child(action->row(),COL_KEY);
			QKeySequence activeKey = descriptor.userKey.isEmpty() ? descriptor.defaultKey : descriptor.userKey;
			key->setText(activeKey.toString(QKeySequence::NativeText));
			key->setData(activeKey,MDR_KEYSEQUENCE);
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
		ShortcutDescriptor descriptor = Shortcuts::shortcutDescriptor(shortcut);
		if (!descriptor.description.isEmpty())
		{
			QStandardItem *action = createTreeRow(shortcut,FModel.invisibleRootItem(),false);
			action->setText(descriptor.description);

			QStandardItem *key = action->parent()->child(action->row(),COL_KEY);
			key->setFlags(Qt::ItemIsEnabled|Qt::ItemIsSelectable|Qt::ItemIsEditable);
			key->setData(shortcut,MDR_SHORTCUTID);
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

void ShortcutOptionsWidget::onResetDefaultsClicked()
{
	foreach(QString shortcut, Shortcuts::shortcuts())
	{
		QStandardItem *action = FShortcutItem.value(shortcut);
		if (action)
		{
			ShortcutDescriptor descriptor = Shortcuts::shortcutDescriptor(shortcut);
			QStandardItem *key = action->parent()->child(action->row(),COL_KEY);
			key->setText(descriptor.defaultKey.toString(QKeySequence::NativeText));
			key->setData(descriptor.defaultKey,MDR_KEYSEQUENCE);
		}
	}
}
