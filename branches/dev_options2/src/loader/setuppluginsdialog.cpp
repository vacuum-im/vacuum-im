#include "setuppluginsdialog.h"

#include <QUuid>
#include <QTimer>
#include <QCursor>
#include <QToolTip>
#include <QMessageBox>
#include <QHeaderView>
#include <QTextDocument>
#include <QDesktopServices>
#include <definitions/menuicons.h>
#include <definitions/resources.h>
#include <definitions/statisticsparams.h>
#include <utils/advanceditemdelegate.h>
#include <utils/iconstorage.h>
#include <utils/options.h>
#include <utils/logger.h>

enum PluginItemDataRoles {
	PDR_LABELS = Qt::UserRole + 1000,
	PDR_UUID,
	PDR_FILE,
	PDR_NAME,
	PDR_DESCR,
	PDR_VERSION,
	PDR_HOMEPAGE,
	PDR_ISLOADED,
	PDR_ISENABLED,
	PDR_DEPENDS_ON,
	PDR_DEPENDS_FOR,
	PDR_DEPENDS_FAILED,
	PDR_FILTER
};


PluginsFilterProxyModel::PluginsFilterProxyModel( QObject *AParent) : QSortFilterProxyModel(AParent)	
{
	FErrorsOnly = false;
	FDisableOnly = false;
}

void PluginsFilterProxyModel::setErrorsOnly(bool AErrors)
{
	if (AErrors != FErrorsOnly)
		FErrorsOnly = AErrors;
}

void PluginsFilterProxyModel::setDisabledOnly(bool ADisabled)
{
	if (ADisabled != FDisableOnly)
		FDisableOnly = ADisabled;
}

bool PluginsFilterProxyModel::filterAcceptsRow(int ASourceRow, const QModelIndex &ASourceParent) const
{
	QModelIndex index = sourceModel()->index(ASourceRow,0,ASourceParent);
	
	if (FErrorsOnly && (index.data(PDR_ISLOADED).toBool() || !index.data(PDR_ISENABLED).toBool()))
		return false;

	if (FDisableOnly && index.data(PDR_ISENABLED).toBool())
		return false;

	return QSortFilterProxyModel::filterAcceptsRow(ASourceRow,ASourceParent);
}

SetupPluginsDialog::SetupPluginsDialog(IPluginManager *APluginManager, QDomDocument APluginsSetup, QWidget *AParent) : QDialog(AParent)
{
	REPORT_VIEW;
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose, true);
	IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(this,MNI_PLUGINMANAGER_SETUP,0,0,"windowIcon");

	FPluginManager = APluginManager;
	FPluginsSetup = APluginsSetup;

	FModel.setColumnCount(1);
	FProxy.setSourceModel(&FModel);
	FProxy.setFilterRole(PDR_FILTER);
	FProxy.setFilterCaseSensitivity(Qt::CaseInsensitive);

	ui.tbvPlugins->setModel(&FProxy);
	ui.tbvPlugins->verticalHeader()->hide();
	ui.tbvPlugins->horizontalHeader()->hide();
	ui.tbvPlugins->setItemDelegate(new AdvancedItemDelegate(ui.tbvPlugins));

	connect(ui.sleSearch,SIGNAL(searchStart()),SLOT(onSearchLineEditSearchStart()));
	connect(ui.chbDisabled,SIGNAL(clicked(bool)),SLOT(onDisabledCheckButtonClicked()));
	connect(ui.chbWithErrors,SIGNAL(clicked(bool)),SLOT(onWithErrorsCheckButtonClicked()));
	connect(ui.dbbButtons,SIGNAL(clicked(QAbstractButton *)),SLOT(onDialogButtonClicked(QAbstractButton *)));
	connect(ui.lblHomePage, SIGNAL(linkActivated(const QString &)),SLOT(onHomePageLinkActivated(const QString &)));
	connect(ui.lblDependsOn, SIGNAL(linkActivated(const QString &)),SLOT(onDependsLinkActivated(const QString &)));
	connect(ui.lblDependsFor, SIGNAL(linkActivated(const QString &)),SLOT(onDependsLinkActivated(const QString &)));
	connect(ui.tbvPlugins->selectionModel(),SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)),SLOT(onCurrentPluginChanged(const QModelIndex &, const QModelIndex &)));

	restoreGeometry(Options::fileValue("misc.setup-plugins-dialog.geometry").toByteArray());
	
	updatePlugins();
}

SetupPluginsDialog::~SetupPluginsDialog()
{
	Options::setFileValue(saveGeometry(),"misc.setup-plugins-dialog.geometry");
}

void SetupPluginsDialog::updatePlugins()
{
	FModel.clear();

	int errorsCount = 0;
	int disableCount = 0;
	QDomElement pluginElem = FPluginsSetup.documentElement().firstChildElement();
	while (!pluginElem.isNull())
	{
		QUuid uuid = pluginElem.attribute("uuid");
		QString file = pluginElem.tagName();
		QString name = pluginElem.firstChildElement("name").text();
		QString version = pluginElem.firstChildElement("version").text();
		QString error = pluginElem.firstChildElement("error").text();
		QString descr = pluginElem.firstChildElement("desc").text();
		bool isEnabled = pluginElem.attribute("enabled","true") == "true";

		const IPluginInfo *info = FPluginManager->pluginInfo(uuid);
		bool isLoaded = info!=NULL;
		QString homePage = info!=NULL ? info->homePage.toString() : QString::null;

		QStringList dependsOn, dependsFor, dependsFailed;
		if (info != NULL)
		{
			foreach(const QUuid &dependUid, FPluginManager->pluginDependencesOn(uuid))
				dependsOn.append(dependUid.toString());
			foreach(const QUuid &dependUid, FPluginManager->pluginDependencesFor(uuid))
				dependsFor.append(dependUid.toString());
		}

		QStandardItem *pluginItem = new QStandardItem(name);
		pluginItem->setData(uuid.toString(), PDR_UUID);
		pluginItem->setData(file, PDR_FILE);
		pluginItem->setData(name.isEmpty() ? file : name, PDR_NAME);
		pluginItem->setData(error.isEmpty() ? descr : error, PDR_DESCR);
		pluginItem->setData(version, PDR_VERSION);
		pluginItem->setData(homePage, PDR_HOMEPAGE);
		pluginItem->setData(isLoaded, PDR_ISLOADED);
		pluginItem->setData(isEnabled, PDR_ISENABLED);
		pluginItem->setData(dependsOn, PDR_DEPENDS_ON);
		pluginItem->setData(dependsFor, PDR_DEPENDS_FOR);
		pluginItem->setData(file+" "+name+" "+descr+" "+error, PDR_FILTER);
		pluginItem->setData(pluginItem->data(PDR_DESCR), Qt::ToolTipRole);
		
		AdvancedDelegateItem nameLabel(AdvancedDelegateItem::DisplayId);
		nameLabel.d->kind = AdvancedDelegateItem::Display;
		nameLabel.d->data = pluginItem->data(PDR_NAME);
		nameLabel.d->hints.insert(AdvancedDelegateItem::FontWeight,QFont::Bold);

		AdvancedDelegateItem versionLabel(AdvancedDelegateItem::DisplayId+1);
		versionLabel.d->kind = AdvancedDelegateItem::CustomData;
		versionLabel.d->data = pluginItem->data(PDR_VERSION);

		AdvancedDelegateItem descrLabel(AdvancedDelegateItem::makeId(AdvancedDelegateItem::Bottom,129,10));
		descrLabel.d->kind = AdvancedDelegateItem::CustomData;
		descrLabel.d->data = pluginItem->data(PDR_DESCR);

		AdvancedDelegateItems labels;
		labels.insert(nameLabel.d->id, nameLabel);
		labels.insert(versionLabel.d->id, versionLabel);
		labels.insert(descrLabel.d->id, descrLabel);
		pluginItem->setData(QVariant::fromValue<AdvancedDelegateItems>(labels), PDR_LABELS);

		if (!isEnabled)
		{
			disableCount++;
			nameLabel.d->hints.insert(AdvancedDelegateItem::Foreground, ui.tbvPlugins->palette().color(QPalette::Disabled, QPalette::Text));
		}
		else if (!isLoaded)
		{
			errorsCount++;
			descrLabel.d->hints.insert(AdvancedDelegateItem::Foreground, Qt::red);
			nameLabel.d->hints.insert(AdvancedDelegateItem::Foreground, ui.tbvPlugins->palette().color(QPalette::Disabled, QPalette::Text));
		}

		pluginItem->setCheckable(true);
		pluginItem->setCheckState(isEnabled ? Qt::Checked : Qt::Unchecked);

		FModel.appendRow(pluginItem);
		FPluginItem.insert(uuid, pluginItem);
		FItemElement.insert(pluginItem, pluginElem);

		pluginElem = pluginElem.nextSiblingElement();
	}
	ui.tbvPlugins->horizontalHeader()->setResizeMode(0,QHeaderView::Stretch);
	ui.tbvPlugins->verticalHeader()->resizeSections(QHeaderView::ResizeToContents);

	FModel.sort(0);
	ui.tbvPlugins->setFocus();
	ui.tbvPlugins->selectRow(0);

	ui.chbDisabled->setEnabled(disableCount > 0);
	ui.chbDisabled->setText(tr("Disabled (%1)").arg(disableCount));

	ui.chbWithErrors->setEnabled(errorsCount > 0);
	ui.chbWithErrors->setText(tr("With errors (%1)").arg(errorsCount));
}

void SetupPluginsDialog::saveSettings()
{
	for (QMap<QStandardItem *, QDomElement>::iterator it = FItemElement.begin(); it!=FItemElement.end(); ++it)
	{
		QDomElement pluginElem = it.value();
		if (it.key()->checkState() == Qt::Checked)
			it->removeAttribute("enabled");
		else
			it->setAttribute("enabled","false");
	}
}

void SetupPluginsDialog::onCurrentPluginChanged(const QModelIndex &ACurrent, const QModelIndex &APrevious)
{
	Q_UNUSED(APrevious);
	if (ACurrent.isValid())
	{
		ui.lblName->setText(QString("<b>%1</b> %2 (%3)").arg(
			Qt::escape(ACurrent.data(PDR_NAME).toString()),
			ACurrent.data(PDR_VERSION).toString(),
			ACurrent.data(PDR_FILE).toString()));

		ui.lblHomePage->setText(QString("<a href='%1'>%2</a>").arg(
			ACurrent.data(PDR_HOMEPAGE).toString(),
			Qt::escape(ACurrent.data(PDR_HOMEPAGE).toString())));

		ui.lblDescription->setText(ACurrent.data(PDR_DESCR).toString());

		if (ACurrent.data(PDR_ISLOADED).toBool())
		{
			QStringList dependsFor = ACurrent.data(PDR_DEPENDS_FOR).toStringList();
			if (!dependsFor.isEmpty())
				ui.lblDependsFor->setText(QString("<a href='depend-for'>%1</a>").arg(tr("This plugin depends on %n other plugins.","",dependsFor.count())));
			else
				ui.lblDependsFor->setText(tr("This plugin does not depend on other plugins."));

			QStringList dependsOn = ACurrent.data(PDR_DEPENDS_ON).toStringList();
			if (!dependsOn.isEmpty())
				ui.lblDependsOn->setText(QString("<a href='depend-on'>%1</a>").arg(tr("Other %n plugins depend on this plugin.","",dependsOn.count())));
			else
				ui.lblDependsOn->setText(tr("Other plugins don't depend on this plugin."));
		}
		else
		{
			QStringList dependsFailed;
			QStandardItem *pluginItem = FPluginItem.value(ACurrent.data(PDR_UUID).toString());
			QDomElement pluginElem = FItemElement.value(pluginItem);
			QDomElement dependElem = pluginElem.firstChildElement("depends").firstChildElement("uuid");
			while (!dependElem.isNull())
			{
				QStandardItem *dependItem = FPluginItem.value(dependElem.text());
				if (dependItem==NULL || !dependItem->data(PDR_ISLOADED).toBool())
					dependsFailed.append(dependItem!=NULL ? dependItem->data(PDR_UUID).toString() : dependElem.text());

				dependElem = dependElem.nextSiblingElement();
			}
			pluginItem->setData(dependsFailed,PDR_DEPENDS_FAILED);

			if (!dependsFailed.isEmpty())
				ui.lblDependsFor->setText(QString("<a href='depend-failed'>%1</a>").arg(tr("Not found %n dependences.","",dependsFailed.count())));
			else
				ui.lblDependsFor->setText(QString::null);
			ui.lblDependsOn->setText(QString::null);
		}
	}
}

void SetupPluginsDialog::onDialogButtonClicked(QAbstractButton *AButton)
{
	switch (ui.dbbButtons->standardButton(AButton))
	{
	case QDialogButtonBox::Ok:
		saveSettings();
		if (QMessageBox::question(this,tr("Restart Application"),tr("Settings saved. Do you want to restart application?"),QMessageBox::Yes|QMessageBox::No) == QMessageBox::Yes)
		{
			REPORT_EVENT(SEVP_APPLICATION_RESTART,1);
			QTimer::singleShot(0,FPluginManager->instance(),SLOT(restart()));
		}
		accept();
		break;
	default:
		reject();
	}
}

void SetupPluginsDialog::onHomePageLinkActivated(const QString &ALink)
{
	QDesktopServices::openUrl(ALink);
}

void SetupPluginsDialog::onDependsLinkActivated(const QString &ALink)
{
	QModelIndex index = ui.tbvPlugins->selectionModel()->currentIndex();
	if (index.isValid())
	{
		QStringList depends;
		if (ALink == "depend-on")
			depends = index.data(PDR_DEPENDS_ON).toStringList();
		else if (ALink == "depend-for")
			depends = index.data(PDR_DEPENDS_FOR).toStringList();
		else if (ALink == "depend-failed")
			depends = index.data(PDR_DEPENDS_FAILED).toStringList();

		QStringList tooltip;
		foreach(const QString &dependUid, depends)
		{
			QStandardItem *pluginItem = FPluginItem.value(dependUid);
			if (pluginItem)
				tooltip.append(pluginItem->data(PDR_NAME).toString());
			else
				tooltip.append(dependUid);
		}

		qSort(tooltip.begin(),tooltip.end());
		QToolTip::showText(QCursor::pos(),tooltip.join("\n"),this);
	}
}

void SetupPluginsDialog::onSearchLineEditSearchStart()
{
	FProxy.setFilterFixedString(ui.sleSearch->text());
	ui.tbvPlugins->verticalHeader()->resizeSections(QHeaderView::ResizeToContents);
}

void SetupPluginsDialog::onDisabledCheckButtonClicked()
{
	ui.chbWithErrors->setChecked(false);
	FProxy.setErrorsOnly(false);
	FProxy.setDisabledOnly(ui.chbDisabled->isChecked());
	FProxy.invalidate();
}

void SetupPluginsDialog::onWithErrorsCheckButtonClicked()
{
	ui.chbDisabled->setChecked(false);
	FProxy.setDisabledOnly(false);
	FProxy.setErrorsOnly(ui.chbWithErrors->isChecked());
	FProxy.invalidate();
}
