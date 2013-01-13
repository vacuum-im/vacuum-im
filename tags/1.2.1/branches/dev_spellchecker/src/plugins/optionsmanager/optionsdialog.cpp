#include "optionsdialog.h"

#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QTextDocument>

static const QString NodeDelimiter = ".";

#define IDR_ORDER   Qt::UserRole + 1

bool SortFilterProxyModel::lessThan(const QModelIndex &ALeft, const QModelIndex &ARight) const
{
	if (ALeft.data(IDR_ORDER).toInt() != ARight.data(IDR_ORDER).toInt())
		return ALeft.data(IDR_ORDER).toInt() < ARight.data(IDR_ORDER).toInt();
	return QSortFilterProxyModel::lessThan(ALeft,ARight);
}

OptionsDialog::OptionsDialog(IOptionsManager *AOptionsManager, QWidget *AParent) : QDialog(AParent)
{
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose,true);
	setWindowTitle(tr("Options"));
	IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(this,MNI_OPTIONS_DIALOG,0,0,"windowIcon");

	if (!restoreGeometry(Options::fileValue("optionsmanager.optionsdialog.geometry").toByteArray()))
		setGeometry(WidgetManager::alignGeometry(QSize(600,520),this));
	if (!ui.sprSplitter->restoreState(Options::fileValue("optionsmanager.optionsdialog.splitter.state").toByteArray()))
		ui.sprSplitter->setSizes(QList<int>() << 160 << 440);

	delete ui.scaScroll->takeWidget();
	ui.trvNodes->sortByColumn(0,Qt::AscendingOrder);

	FOptionsManager = AOptionsManager;
	connect(FOptionsManager->instance(),SIGNAL(optionsDialogNodeInserted(const IOptionsDialogNode &)),SLOT(onOptionsDialogNodeInserted(const IOptionsDialogNode &)));
	connect(FOptionsManager->instance(),SIGNAL(optionsDialogNodeRemoved(const IOptionsDialogNode &)),SLOT(onOptionsDialogNodeRemoved(const IOptionsDialogNode &)));

	FItemsModel = new QStandardItemModel(ui.trvNodes);
	FItemsModel->setColumnCount(1);

	FProxyModel = new SortFilterProxyModel(FItemsModel);
	FProxyModel->setSourceModel(FItemsModel);
	FProxyModel->setSortLocaleAware(true);
	FProxyModel->setDynamicSortFilter(true);
	FProxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);

	ui.trvNodes->setModel(FProxyModel);
	connect(ui.trvNodes->selectionModel(),SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)),
	        SLOT(onCurrentItemChanged(const QModelIndex &, const QModelIndex &)));

	ui.dbbButtons->button(QDialogButtonBox::Apply)->setEnabled(false);
	ui.dbbButtons->button(QDialogButtonBox::Reset)->setEnabled(false);
	connect(ui.dbbButtons,SIGNAL(clicked(QAbstractButton *)),SLOT(onDialogButtonClicked(QAbstractButton *)));

	foreach (const IOptionsDialogNode &node, FOptionsManager->optionsDialogNodes()) {
		onOptionsDialogNodeInserted(node); }
}

OptionsDialog::~OptionsDialog()
{
	Options::setFileValue(saveGeometry(),"optionsmanager.optionsdialog.geometry");
	Options::setFileValue(ui.sprSplitter->saveState(),"optionsmanager.optionsdialog.splitter.state");

	disconnect(FOptionsManager->instance(), 0, this, 0);
	disconnect(ui.trvNodes->selectionModel(), 0, this, 0);
	FCleanupHandler.clear();
}

void OptionsDialog::showNode(const QString &ANodeId)
{
	QStandardItem *item = FNodeItems.value(ANodeId, NULL);
	if (item)
		ui.trvNodes->setCurrentIndex(FProxyModel->mapFromSource(FItemsModel->indexFromItem(item)));
	ui.trvNodes->expandAll();
}

QWidget *OptionsDialog::createNodeWidget(const QString &ANodeId)
{
	QWidget *nodeWidget = new QWidget(ui.scaScroll);
	nodeWidget->setLayout(new QVBoxLayout);
	nodeWidget->layout()->setMargin(5);

	QMultiMap<int, IOptionsWidget *> orderedWidgets;
	foreach(IOptionsHolder *optionsHolder,FOptionsManager->optionsHolders())
	{
		QMultiMap<int, IOptionsWidget *> widgets = optionsHolder->optionsWidgets(ANodeId,nodeWidget);
		for (QMultiMap<int, IOptionsWidget *>::const_iterator  it = widgets.constBegin(); it!=widgets.constEnd(); ++it)
		{
			orderedWidgets.insertMulti(it.key() ,it.value());
			connect(this,SIGNAL(applied()),it.value()->instance(),SLOT(apply()));
			connect(this,SIGNAL(reseted()),it.value()->instance(),SLOT(reset()));
			connect(it.value()->instance(),SIGNAL(modified()),SLOT(onOptionsWidgetModified()));
		}
	}

	if (!orderedWidgets.isEmpty())
	{
		foreach(IOptionsWidget *widget, orderedWidgets)
			nodeWidget->layout()->addWidget(widget->instance());
		if (!canExpandVertically(nodeWidget))
			nodeWidget->setMaximumHeight(nodeWidget->sizeHint().height());
	}
	else
	{
		QLabel *label = new QLabel(tr("Options are absent"),nodeWidget);
		label->setAlignment(Qt::AlignCenter);
		label->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
		nodeWidget->layout()->addWidget(label);
	}

	FCleanupHandler.add(nodeWidget);
	return nodeWidget;
}

QStandardItem *OptionsDialog::createNodeItem(const QString &ANodeID)
{
	QString curNodeID;
	QStandardItem *item = NULL;
	foreach(QString nodeID, ANodeID.split(NodeDelimiter,QString::SkipEmptyParts))
	{
		if (curNodeID.isEmpty())
			curNodeID = nodeID;
		else
			curNodeID += NodeDelimiter+nodeID;

		if (!FNodeItems.contains(curNodeID))
		{
			if (item)
			{
				QStandardItem *newlItem = new QStandardItem(nodeID);
				item->appendRow(newlItem);
				item = newlItem;
			}
			else
			{
				item = new QStandardItem(nodeID);
				FItemsModel->appendRow(item);
			}
			FNodeItems.insert(curNodeID,item);
		}
		else
		{
			item = FNodeItems.value(curNodeID);
		}
	}
	return item;
}

bool OptionsDialog::canExpandVertically(const QWidget *AWidget) const
{
	bool expanding = AWidget->sizePolicy().verticalPolicy() == QSizePolicy::Expanding;
	if (!expanding)
	{
		QObjectList childs = AWidget->children();
		for (int i=0; !expanding && i<childs.count(); i++)
			if (childs.at(i)->isWidgetType())
				expanding = canExpandVertically(qobject_cast<QWidget *>(childs.at(i)));
	}
	return expanding;
}

void OptionsDialog::onOptionsDialogNodeInserted(const IOptionsDialogNode &ANode)
{
	if (!ANode.nodeId.isEmpty() && !ANode.name.isEmpty())
	{
		QStandardItem *item = FNodeItems.contains(ANode.nodeId) ? FNodeItems.value(ANode.nodeId) : createNodeItem(ANode.nodeId);
		item->setData(ANode.order, IDR_ORDER);
		item->setText(ANode.name);
		item->setIcon(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(ANode.iconkey));
	}
}

void OptionsDialog::onOptionsDialogNodeRemoved(const IOptionsDialogNode &ANode)
{
	if (FNodeItems.contains(ANode.nodeId))
	{
		foreach(QString nodeId, FNodeItems.keys())
		{
			if (nodeId.left(nodeId.lastIndexOf('.')+1) == ANode.nodeId+".")
			{
				IOptionsDialogNode childNode;
				childNode.nodeId = nodeId;
				onOptionsDialogNodeRemoved(childNode);
			}
		}

		QStandardItem *item = FNodeItems.take(ANode.nodeId);
		if (item->parent())
			item->parent()->removeRow(item->row());
		else
			qDeleteAll(FItemsModel->takeRow(item->row()));
		delete FItemWidgets.take(item);
	}
}

void OptionsDialog::onCurrentItemChanged(const QModelIndex &ACurrent, const QModelIndex &APrevious)
{
	Q_UNUSED(APrevious);
	ui.scaScroll->takeWidget();

	QStandardItem *curItem = FItemsModel->itemFromIndex(FProxyModel->mapToSource(ACurrent));
	QString nodeID = FNodeItems.key(curItem);
	if (curItem && !FItemWidgets.contains(curItem))
		FItemWidgets.insert(curItem,createNodeWidget(nodeID));

	QWidget *curWidget = FItemWidgets.value(curItem);
	if (curWidget)
		ui.scaScroll->setWidget(curWidget);

	Options::node(OPV_MISC_OPTIONS_DIALOG_LASTNODE).setValue(nodeID);
}

void OptionsDialog::onOptionsWidgetModified()
{
	ui.dbbButtons->button(QDialogButtonBox::Apply)->setEnabled(true);
	ui.dbbButtons->button(QDialogButtonBox::Reset)->setEnabled(true);
}

void OptionsDialog::onDialogButtonClicked(QAbstractButton *AButton)
{
	switch (ui.dbbButtons->buttonRole(AButton))
	{
	case QDialogButtonBox::AcceptRole:
		emit applied();
		accept();
		break;
	case QDialogButtonBox::ApplyRole:
		emit applied();
		ui.dbbButtons->button(QDialogButtonBox::Apply)->setEnabled(false);
		ui.dbbButtons->button(QDialogButtonBox::Reset)->setEnabled(false);
		break;
	case QDialogButtonBox::ResetRole:
		emit reseted();
		ui.dbbButtons->button(QDialogButtonBox::Apply)->setEnabled(false);
		ui.dbbButtons->button(QDialogButtonBox::Reset)->setEnabled(false);
		break;
	case QDialogButtonBox::RejectRole:
		reject();
		break;
	default:
		break;
	}
}
