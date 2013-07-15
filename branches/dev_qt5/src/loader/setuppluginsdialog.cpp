#include "setuppluginsdialog.h"

#include <QTimer>
#include <QMessageBox>
#include <QHeaderView>
#include <utils/QtEscape.h>
#include <QDesktopServices>

enum TableColumns
{
	COL_NAME,
	COL_FILE
};

SetupPluginsDialog::SetupPluginsDialog(IPluginManager *APluginManager, QDomDocument APluginsSetup, QWidget *AParent) : QDialog(AParent)
{
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose, true);
	IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(this,MNI_PLUGINMANAGER_SETUP,0,0,"windowIcon");

	FPluginManager = APluginManager;
	FPluginsSetup = APluginsSetup;

	updateLanguage();
	connect(ui.cmbLanguage,SIGNAL(currentIndexChanged(int)),SLOT(onCurrentLanguageChanged(int)));

	updatePlugins(); 
#if QT_VERSION < 0x050000
	ui.twtPlugins->horizontalHeader()->setResizeMode(COL_NAME,QHeaderView::Stretch);
	ui.twtPlugins->horizontalHeader()->setResizeMode(COL_FILE,QHeaderView::ResizeToContents);
#else
	ui.twtPlugins->horizontalHeader()->setSectionResizeMode(COL_NAME,QHeaderView::Stretch);
	ui.twtPlugins->horizontalHeader()->setSectionResizeMode(COL_FILE,QHeaderView::ResizeToContents);
#endif
	connect(ui.twtPlugins,SIGNAL(currentItemChanged(QTableWidgetItem *, QTableWidgetItem *)),SLOT(onCurrentPluginChanged(QTableWidgetItem *, QTableWidgetItem *)));

	connect(ui.dbbButtons,SIGNAL(clicked(QAbstractButton *)),SLOT(onDialogButtonClicked(QAbstractButton *)));
	connect(ui.lblHomePage, SIGNAL(linkActivated(const QString &)),SLOT(onHomePageLinkActivated(const QString &)));

	restoreGeometry(Options::fileValue("misc.setup-plugins-dialog.geometry").toByteArray());
}

SetupPluginsDialog::~SetupPluginsDialog()
{
	Options::setFileValue(saveGeometry(),"misc.setup-plugins-dialog.geometry");
}

void SetupPluginsDialog::updateLanguage()
{
	ui.cmbLanguage->clear();
	for (int lang = QLocale::C+1; lang <= QLocale::Chewa; lang++)
		ui.cmbLanguage->addItem(QLocale::languageToString((QLocale::Language)lang), lang);
	ui.cmbLanguage->model()->sort(0, Qt::AscendingOrder);
	ui.cmbLanguage->insertItem(0,tr("<System>"),QLocale::C);

	QLocale locale;
	if (locale != QLocale::system())
		ui.cmbLanguage->setCurrentIndex(ui.cmbLanguage->findData((int)locale.language()));
	else
		ui.cmbLanguage->setCurrentIndex(0);
	onCurrentLanguageChanged(ui.cmbLanguage->currentIndex());
}

void SetupPluginsDialog::updatePlugins()
{
	FItemElement.clear();
	ui.twtPlugins->clearContents();
	ui.twtPlugins->setRowCount(0);

	QDomElement pluginElem = FPluginsSetup.documentElement().firstChildElement();
	while (!pluginElem.isNull())
	{
		QString name = pluginElem.firstChildElement("name").text().isEmpty() ? pluginElem.tagName() : pluginElem.firstChildElement("name").text();
		QTableWidgetItem *nameItem = new QTableWidgetItem(name);
		if (pluginElem.attribute("enabled","true")=="true")
		{
			if (FPluginManager->pluginInstance(pluginElem.attribute("uuid"))==NULL)
				nameItem->setForeground(Qt::red);
			nameItem->setCheckState(Qt::Checked);
		}
		else
		{
			nameItem->setForeground(Qt::gray);
			nameItem->setCheckState(Qt::Unchecked);
		}
		nameItem->setCheckState(pluginElem.attribute("enabled","true")=="true" ? Qt::Checked : Qt::Unchecked);

		QTableWidgetItem *fileItem = new QTableWidgetItem(pluginElem.tagName());

		ui.twtPlugins->setRowCount(ui.twtPlugins->rowCount()+1);
		ui.twtPlugins->setItem(ui.twtPlugins->rowCount()-1, COL_NAME, nameItem);
		ui.twtPlugins->setItem(nameItem->row(), COL_FILE, fileItem);
		
		FItemElement.insert(nameItem,pluginElem);
		pluginElem = pluginElem.nextSiblingElement();
	}
	ui.twtPlugins->sortItems(COL_NAME,Qt::AscendingOrder);
}

void SetupPluginsDialog::saveSettings()
{
	QMap<QTableWidgetItem *, QDomElement>::iterator it = FItemElement.begin();
	while (it != FItemElement.end())
	{
		QDomElement pluginElem = it.value();
		if (it.key()->checkState() == Qt::Checked)
			pluginElem.removeAttribute("enabled");
		else
			pluginElem.setAttribute("enabled","false");
		++it;
	}
	FPluginManager->setLocale((QLocale::Language)ui.cmbLanguage->itemData(ui.cmbLanguage->currentIndex()).toInt(), (QLocale::Country)ui.cmbCountry->itemData(ui.cmbCountry->currentIndex()).toInt());
}

QDomElement SetupPluginsDialog::getPluginElement(const QUuid &AUuid) const
{
	QDomElement pluginElem = FPluginsSetup.documentElement().firstChildElement();
	while (!pluginElem.isNull() && AUuid!=pluginElem.attribute("uuid"))
		pluginElem = pluginElem.nextSiblingElement();
	return pluginElem;
}

void SetupPluginsDialog::onCurrentLanguageChanged(int AIndex)
{
	ui.cmbCountry->clear();
	QLocale::Language lang = (QLocale::Language)ui.cmbLanguage->itemData(AIndex).toInt();
	foreach (QLocale::Country country, QLocale::countriesForLanguage(lang))
		ui.cmbCountry->addItem(QLocale::countryToString(country),(int)country);
	ui.cmbCountry->model()->sort(0, Qt::AscendingOrder);

	if (lang != QLocale::C)
		ui.cmbCountry->insertItem(0,tr("<Any Country>"), QLocale::AnyCountry);

	QLocale locale;
	if (locale.language() == lang)
		ui.cmbCountry->setCurrentIndex(ui.cmbCountry->findData((int)locale.country()));
	else
		ui.cmbCountry->setCurrentIndex(0);
}

void SetupPluginsDialog::onCurrentPluginChanged(QTableWidgetItem *ACurrent, QTableWidgetItem *APrevious)
{
	Q_UNUSED(APrevious);
	QTableWidgetItem *nameItem = ACurrent==NULL || ACurrent->column()==COL_NAME ? ACurrent : ui.twtPlugins->item(ACurrent->row(), COL_NAME);
	if (FItemElement.contains(nameItem))
	{
		QDomElement pluginElem = FItemElement.value(nameItem);

		QString name = pluginElem.firstChildElement("name").text().isEmpty() ? pluginElem.tagName() : pluginElem.firstChildElement("name").text();
		ui.lblName->setText(QString("<b>%1</b> %2").arg(Qt::escape(name)).arg(Qt::escape(pluginElem.firstChildElement("version").text())));
		ui.lblDescription->setText(pluginElem.firstChildElement("desc").text());
		ui.lblError->setText(pluginElem.firstChildElement("error").text());
		ui.lblError->setVisible(!ui.lblError->text().isEmpty());
		ui.lblLabelError->setVisible(ui.lblError->isVisible());

		ui.ltwDepends->clear();
		QDomElement dependsElem = pluginElem.firstChildElement("depends").firstChildElement("uuid");
		while (!dependsElem.isNull())
		{
			QDomElement dpluginElem = getPluginElement(dependsElem.text());
			QListWidgetItem *dItem = new QListWidgetItem(!dpluginElem.isNull() ? dpluginElem.firstChildElement("name").text() : dependsElem.text());
			QPalette::ColorGroup cg = FPluginManager->pluginInstance(dependsElem.text())!=NULL ? QPalette::Active : QPalette::Disabled;
			dItem->setForeground(ui.ltwDepends->palette().color(cg, QPalette::Text));
			ui.ltwDepends->addItem(dItem);
			dependsElem = dependsElem.nextSiblingElement("uuid");
		}

		ui.ltwDepend->clear();
		QDomElement dpluginElem = FPluginsSetup.documentElement().firstChildElement();
		while (!dpluginElem.isNull())
		{
			QDomElement dependsElem = dpluginElem.firstChildElement("depends").firstChildElement("uuid");
			while (!dependsElem.isNull())
			{
				if (pluginElem.attribute("uuid") == dependsElem.text())
				{
					ui.ltwDepend->addItem(dpluginElem.firstChildElement("name").text());
					break;
				}
				dependsElem = dependsElem.firstChildElement("uuid");
			}
			dpluginElem = dpluginElem.nextSiblingElement();
		}

		const IPluginInfo *info = FPluginManager->pluginInfo(pluginElem.attribute("uuid"));
		if (info)
			ui.lblHomePage->setText(QString("<a href='%1'>%2</a>").arg(info->homePage.toString()).arg(Qt::escape(info->homePage.toString())));
	}
}

void SetupPluginsDialog::onDialogButtonClicked(QAbstractButton *AButton)
{
	switch (ui.dbbButtons->standardButton(AButton))
	{
	case QDialogButtonBox::Ok:
		saveSettings();
		if (QMessageBox::question(this,tr("Restart Application"),tr("Settings saved. Do you want to restart application?"),QMessageBox::Yes|QMessageBox::No) == QMessageBox::Yes)
			QTimer::singleShot(0,FPluginManager->instance(), SLOT(restart()));
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
