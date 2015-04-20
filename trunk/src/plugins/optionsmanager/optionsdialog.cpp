#include "optionsdialog.h"

#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QTextDocument>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/optionvalues.h>
#include <utils/widgetmanager.h>
#include <utils/iconstorage.h>
#include <utils/options.h>
#include <utils/logger.h>
#include "optionsdialogheader.h"

#define IDR_ORDER  Qt::UserRole + 1

#define NODE_ITEM_ICONSIZE   QSize(16,16)
#define NODE_ITEM_SIZEHINT   QSize(24,24)

static const QString NodeDelimiter = ".";

bool SortFilterProxyModel::lessThan(const QModelIndex &ALeft, const QModelIndex &ARight) const
{
	if (ALeft.data(IDR_ORDER).toInt() != ARight.data(IDR_ORDER).toInt())
		return ALeft.data(IDR_ORDER).toInt() < ARight.data(IDR_ORDER).toInt();
	return QSortFilterProxyModel::lessThan(ALeft,ARight);
}

OptionsDialog::OptionsDialog(IOptionsManager *AOptionsManager, const QString &ARootId, QWidget *AParent) : QDialog(AParent)
{
	REPORT_VIEW;
	ui.setupUi(this);
	setWindowTitle(tr("Options"));
	setWindowModality(Qt::WindowModal);
	setAttribute(Qt::WA_DeleteOnClose,true);
	IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(this,MNI_OPTIONS_DIALOG,0,0,"windowIcon");

	FRootNodeId = ARootId;
	delete ui.scaScroll->takeWidget();

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
	ui.trvNodes->setIconSize(NODE_ITEM_ICONSIZE);
	ui.trvNodes->setRootIsDecorated(false);
	ui.trvNodes->setUniformRowHeights(false);
	ui.trvNodes->sortByColumn(0,Qt::AscendingOrder);
	connect(ui.trvNodes->selectionModel(),SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)),SLOT(onCurrentItemChanged(const QModelIndex &, const QModelIndex &)));

	ui.dbbButtons->button(QDialogButtonBox::Apply)->setEnabled(false);
	ui.dbbButtons->button(QDialogButtonBox::Reset)->setEnabled(false);
	connect(ui.dbbButtons,SIGNAL(clicked(QAbstractButton *)),SLOT(onDialogButtonClicked(QAbstractButton *)));

	foreach (const IOptionsDialogNode &node, FOptionsManager->optionsDialogNodes())
		onOptionsDialogNodeInserted(node);
	ui.trvNodes->setVisible(FItemsModel->rowCount() > 0);

	if (!restoreGeometry(Options::fileValue("optionsmanager.optionsdialog.geometry",FRootNodeId).toByteArray()))
		setGeometry(WidgetManager::alignGeometry(FItemsModel->rowCount()>0 ? QSize(750,560) : QSize(570,560),this));
	if (!ui.sprSplitter->restoreState(Options::fileValue("optionsmanager.optionsdialog.splitter.state",FRootNodeId).toByteArray()))
		ui.sprSplitter->setSizes(QList<int>() << 180 << 620);
}

OptionsDialog::~OptionsDialog()
{
	Options::setFileValue(saveGeometry(),"optionsmanager.optionsdialog.geometry",FRootNodeId);
	Options::setFileValue(ui.sprSplitter->saveState(),"optionsmanager.optionsdialog.splitter.state",FRootNodeId);

	FCleanupHandler.clear();
}

void OptionsDialog::showNode(const QString &ANodeId)
{
	QStandardItem *item = FNodeItems.value(ANodeId, NULL);
	if (item)
		ui.trvNodes->setCurrentIndex(FProxyModel->mapFromSource(FItemsModel->indexFromItem(item)));
}

QWidget *OptionsDialog::createNodeWidget(const QString &ANodeId)
{
	LOG_DEBUG(QString("Creating options dialog widgets for node=%1").arg(ANodeId));

	QWidget *nodeWidget = new QWidget(ui.scaScroll);
	QVBoxLayout *nodeLayout = new QVBoxLayout(nodeWidget);
	nodeLayout->setMargin(5);

	QMultiMap<int, IOptionsDialogWidget *> orderedWidgets;
	foreach(IOptionsDialogHolder *optionsHolder, FOptionsManager->optionsDialogHolders())
		orderedWidgets += optionsHolder->optionsDialogWidgets(ANodeId,nodeWidget);

	if (!orderedWidgets.isEmpty())
	{
		QVBoxLayout *headerLayout = NULL;
		IOptionsDialogWidget *headerWidget = NULL;
		foreach(IOptionsDialogWidget *widget, orderedWidgets)
		{
			bool isHeader = qobject_cast<OptionsDialogHeader *>(widget->instance()) != NULL;
			if (!isHeader)
			{
				if (headerLayout == NULL)
				{
					headerLayout = new QVBoxLayout;
					headerLayout->setContentsMargins(15,0,0,0);
					nodeLayout->addLayout(headerLayout);
				}
				headerLayout->addWidget(widget->instance());
			}
			else
			{
				if (headerLayout != NULL)
				{
					nodeLayout->addSpacing(10);
					headerLayout = NULL;
				}
				else if (headerWidget != NULL)
				{
					delete headerWidget->instance();
				}
				nodeLayout->addWidget(widget->instance());
				headerWidget = widget;
			}

			connect(this,SIGNAL(applied()),widget->instance(),SLOT(apply()));
			connect(this,SIGNAL(reseted()),widget->instance(),SLOT(reset()));
			connect(widget->instance(),SIGNAL(modified()),SLOT(onOptionsWidgetModified()));
		}

		if (headerWidget!=NULL && headerLayout==NULL)
			delete headerWidget->instance();

		if (!canExpandVertically(nodeWidget))
			nodeLayout->addStretch();
	}
	else
	{
		QLabel *label = new QLabel(tr("Options are absent"),nodeWidget);
		label->setAlignment(Qt::AlignCenter);
		label->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
		nodeLayout->addWidget(label);
	}

	FCleanupHandler.add(nodeWidget);
	return nodeWidget;
}

QStandardItem *OptionsDialog::getNodeModelItem(const QString &ANodeId)
{
	QStandardItem *item = FNodeItems.value(ANodeId);
	if (item == NULL)
	{
		item = new QStandardItem(ANodeId);
		FItemsModel->appendRow(item);
		FNodeItems.insert(ANodeId,item);
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

void OptionsDialog::onOptionsWidgetModified()
{
	ui.dbbButtons->button(QDialogButtonBox::Apply)->setEnabled(true);
	ui.dbbButtons->button(QDialogButtonBox::Reset)->setEnabled(true);
}

void OptionsDialog::onOptionsDialogNodeInserted(const IOptionsDialogNode &ANode)
{
	QString prefix = FRootNodeId + NodeDelimiter;
	if (!ANode.nodeId.isEmpty() && !ANode.caption.isEmpty() && !FRootNodeId.isEmpty() ? ANode.nodeId.startsWith(prefix) : true)
	{
		// Do not show child nodes
		if (ANode.nodeId.indexOf(NodeDelimiter,!FRootNodeId.isEmpty() ? prefix.size()+1 : 0) < 0)
		{
			QStandardItem *item = getNodeModelItem(ANode.nodeId);
			item->setText(ANode.caption);
			item->setData(ANode.order,IDR_ORDER);
			item->setData(NODE_ITEM_SIZEHINT,Qt::SizeHintRole);
			item->setIcon(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(ANode.iconkey));

			ui.trvNodes->setVisible(FItemsModel->rowCount() > 0);
		}
	}
}

void OptionsDialog::onOptionsDialogNodeRemoved(const IOptionsDialogNode &ANode)
{
	if (FNodeItems.contains(ANode.nodeId))
	{
		QStandardItem *item = FNodeItems.take(ANode.nodeId);
		qDeleteAll(FItemsModel->takeRow(item->row()));
		delete FItemWidgets.take(item);

		ui.trvNodes->setVisible(FItemsModel->rowCount() > 0);
	}
	else if (ANode.nodeId == FRootNodeId)
	{
		reject();
	}
}

void OptionsDialog::onCurrentItemChanged(const QModelIndex &ACurrent, const QModelIndex &APrevious)
{
	Q_UNUSED(APrevious);
	ui.scaScroll->takeWidget();

	QStandardItem *curItem = FItemsModel->itemFromIndex(FProxyModel->mapToSource(ACurrent));

	QString nodeId = FNodeItems.key(curItem);
	LOG_DEBUG(QString("Changing current options dialog node to %1").arg(nodeId));

	if (curItem && !FItemWidgets.contains(curItem))
		FItemWidgets.insert(curItem,createNodeWidget(nodeId));

	QWidget *curWidget = FItemWidgets.value(curItem);
	if (curWidget)
		ui.scaScroll->setWidget(curWidget);

	Options::setFileValue(nodeId,"options.dialog.last-node",FRootNodeId);
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
