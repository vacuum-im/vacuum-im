#include "discoinfowindow.h"

#include <QHeaderView>

#define ADR_FORM_INDEX        Action::DR_Parametr1

DiscoInfoWindow::DiscoInfoWindow(IServiceDiscovery *ADiscovery, const Jid &AStreamJid, const Jid &AContactJid,
                                 const QString &ANode, QWidget *AParent) : QDialog(AParent)
{
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose,true);
	setWindowTitle(tr("Discovery Info - %1").arg(AContactJid.full()));
	IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(this,MNI_SDISCOVERY_DISCOINFO,0,0,"windowIcon");

	FDataForms = NULL;
	FDiscovery = ADiscovery;
	FStreamJid = AStreamJid;
	FContactJid = AContactJid;
	FNode = ANode;
	FFormMenu = NULL;

	ui.pbtExtensions->setEnabled(false);
	ui.lblError->setVisible(false);

	initialize();
	connect(FDiscovery->instance(),SIGNAL(discoInfoReceived(const IDiscoInfo &)),
	        SLOT(onDiscoInfoReceived(const IDiscoInfo &)));
	connect(ui.pbtUpdate,SIGNAL(clicked()),SLOT(onUpdateClicked()));
	connect(ui.lwtFearures,SIGNAL(currentItemChanged(QListWidgetItem *, QListWidgetItem *)),
	        SLOT(onCurrentFeatureChanged(QListWidgetItem *, QListWidgetItem *)));
	connect(ui.lwtFearures,SIGNAL(itemActivated(QListWidgetItem *)),SLOT(onListItemActivated(QListWidgetItem *)));

	if (!FDiscovery->hasDiscoInfo(FStreamJid,FContactJid,ANode) || FDiscovery->discoInfo(FStreamJid,FContactJid,ANode).error.code>0)
		requestDiscoInfo();
	else
		updateWindow();
}

DiscoInfoWindow::~DiscoInfoWindow()
{

}

void DiscoInfoWindow::initialize()
{
	IPlugin *plugin = FDiscovery->pluginManager()->pluginInterface("IDataForms").value(0,NULL);
	if (plugin)
		FDataForms = qobject_cast<IDataForms *>(plugin->instance());
}

void DiscoInfoWindow::updateWindow()
{
	IDiscoInfo dinfo = FDiscovery->discoInfo(FStreamJid,FContactJid,FNode);

	int row = 0;
	ui.twtIdentity->clearContents();
	foreach(IDiscoIdentity identity, dinfo.identity)
	{
		ui.twtIdentity->setRowCount(row+1);
		ui.twtIdentity->setItem(row,0,new QTableWidgetItem(identity.category));
		ui.twtIdentity->setItem(row,1,new QTableWidgetItem(identity.type));
		ui.twtIdentity->setItem(row,2,new QTableWidgetItem(identity.name));
		row++;
	}
	ui.twtIdentity->verticalHeader()->resizeSections(QHeaderView::ResizeToContents);

	qSort(dinfo.features);
	ui.lwtFearures->clear();
	foreach(QString featureVar, dinfo.features)
	{
		IDiscoFeature dfeature = FDiscovery->discoFeature(featureVar);
		dfeature.var = featureVar;
		QListWidgetItem *listItem = new QListWidgetItem;
		listItem->setIcon(dfeature.icon);
		listItem->setText(dfeature.name.isEmpty() ? dfeature.var : dfeature.name);
		if (FDiscovery->hasFeatureHandler(featureVar))
		{
			QFont font = ui.lwtFearures->font();
			font.setBold(true);
			listItem->setData(Qt::FontRole,font);
		}
		listItem->setData(Qt::UserRole,dfeature.var);
		listItem->setData(Qt::UserRole+1,dfeature.description);
		ui.lwtFearures->addItem(listItem);
	}
	onCurrentFeatureChanged(ui.lwtFearures->currentItem(), NULL);

	if (FDataForms)
	{
		if (FFormMenu)
		{
			FFormMenu->deleteLater();
			FFormMenu = NULL;
		}
		if (!dinfo.extensions.isEmpty())
		{
			FFormMenu = new Menu(ui.pbtExtensions);
			for (int index=0; index<dinfo.extensions.count(); index++)
			{
				IDataForm form = FDataForms->localizeForm(dinfo.extensions.at(index));
				Action *action = new Action(FFormMenu);
				action->setData(ADR_FORM_INDEX,index);
				action->setText(!form.title.isEmpty() ? form.title : FDataForms->fieldValue("FORM_TYPE",form.fields).toString());
				connect(action,SIGNAL(triggered(bool)),SLOT(onShowExtensionForm(bool)));
				FFormMenu->addAction(action);
			}
		}
		ui.pbtExtensions->setMenu(FFormMenu);
		ui.pbtExtensions->setEnabled(FFormMenu!=NULL);
	}

	if (dinfo.error.code > 0)
	{
		ui.lblError->setText(tr("Error %1: %2").arg(dinfo.error.code).arg(dinfo.error.message));
		ui.twtIdentity->setEnabled(false);
		ui.lwtFearures->setEnabled(false);
		ui.lblError->setVisible(true);
	}
	else
	{
		ui.twtIdentity->setEnabled(true);
		ui.lwtFearures->setEnabled(true);
		ui.lblError->setVisible(false);
	}

	ui.twtIdentity->horizontalHeader()->setResizeMode(0,QHeaderView::ResizeToContents);
	ui.twtIdentity->horizontalHeader()->setResizeMode(1,QHeaderView::ResizeToContents);
	ui.twtIdentity->horizontalHeader()->setResizeMode(2,QHeaderView::Stretch);

	ui.pbtUpdate->setEnabled(true);
}

void DiscoInfoWindow::requestDiscoInfo()
{
	if (FDiscovery->requestDiscoInfo(FStreamJid,FContactJid,FNode))
		ui.pbtUpdate->setEnabled(false);
}

void DiscoInfoWindow::onDiscoInfoReceived(const IDiscoInfo &ADiscoInfo)
{
	if (ADiscoInfo.contactJid == FContactJid)
		updateWindow();
}

void DiscoInfoWindow::onCurrentFeatureChanged(QListWidgetItem *ACurrent, QListWidgetItem *APrevious)
{
	Q_UNUSED(APrevious);
	if (ACurrent)
		ui.lblFeatureDesc->setText(ACurrent->data(Qt::UserRole+1).toString());
	else
		ui.lblFeatureDesc->setText("");
	ui.lblFeatureDesc->setMinimumHeight(ui.lblFeatureDesc->height());
}

void DiscoInfoWindow::onUpdateClicked()
{
	requestDiscoInfo();
}

void DiscoInfoWindow::onListItemActivated(QListWidgetItem *AItem)
{
	QString feature = AItem->data(Qt::UserRole).toString();
	if (FDiscovery->hasFeatureHandler(feature))
	{
		IDiscoInfo dinfo = FDiscovery->discoInfo(FStreamJid,FContactJid,FNode);
		FDiscovery->execFeatureHandler(FStreamJid,feature,dinfo);
	}
}

void DiscoInfoWindow::onShowExtensionForm(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action && FDataForms)
	{
		IDiscoInfo dinfo = FDiscovery->discoInfo(FStreamJid,FContactJid,FNode);
		int index = action->data(ADR_FORM_INDEX).toInt();
		if (index<dinfo.extensions.count())
		{
			IDataForm form = FDataForms->localizeForm(dinfo.extensions.at(index));
			IDataDialogWidget *widget = FDataForms->dialogWidget(form,this);
			widget->dialogButtons()->setStandardButtons(QDialogButtonBox::Ok);
			widget->instance()->setWindowModality(Qt::WindowModal);
			widget->instance()->setWindowTitle(action->text());
			widget->instance()->show();
		}
	}
}
