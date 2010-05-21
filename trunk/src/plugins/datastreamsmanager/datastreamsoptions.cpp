#include "datastreamsoptions.h"

#include <QUuid>
#include <QMessageBox>
#include <QInputDialog>

DataStreamsOptions::DataStreamsOptions(IDataStreamsManager *ADataManager, QWidget *AParent) : QWidget(AParent)
{
	ui.setupUi(this);
	FDataManager = ADataManager;

	FWidgetLayout = new QVBoxLayout;
	FWidgetLayout->setMargin(0);
	ui.wdtSettings->setLayout(FWidgetLayout);

	connect(ui.pbtAddProfile, SIGNAL(clicked(bool)),SLOT(onAddProfileButtonClicked(bool)));
	connect(ui.pbtDeleteProfile, SIGNAL(clicked(bool)),SLOT(onDeleteProfileButtonClicked(bool)));
	connect(ui.cmbProfile, SIGNAL(currentIndexChanged(int)), SLOT(onCurrentProfileChanged(int)));

	reset();
}

DataStreamsOptions::~DataStreamsOptions()
{
	FCleanupHandler.clear();
	foreach(QUuid profileId, FNewProfiles) {
		Options::node(OPV_DATASTREAMS_ROOT).removeChilds("settings-profile",profileId.toString()); }
}

void DataStreamsOptions::apply()
{
	QList<QUuid> oldProfiles = FDataManager->settingsProfiles();

	for (int index=0; index<ui.cmbProfile->count(); index++)
	{
		QUuid profileId = ui.cmbProfile->itemData(index).toString();
		QString name = ui.cmbProfile->itemText(index);
		FDataManager->insertSettingsProfile(profileId, name);

		QMap<QString, IOptionsWidget *> &widgets = FMethodWidgets[profileId];
		foreach(QString smethodNS, widgets.keys())
		{
			IOptionsWidget *widget = widgets.value(smethodNS);
			if (widget)
			{
				IDataStreamMethod *smethod = FDataManager->method(smethodNS);
				if (smethod)
					smethod->saveMethodSettings(widget,FDataManager->settingsProfileNode(profileId,smethodNS));
			}
		}

		oldProfiles.removeAll(profileId);
	}

	foreach(QUuid profileId, oldProfiles)
		FDataManager->removeSettingsProfile(profileId);

	FNewProfiles.clear();

	emit childApply();
}

void DataStreamsOptions::reset()
{
	foreach(QUuid profileId, FNewProfiles)
	{
		foreach(IOptionsWidget *widget, FMethodWidgets.take(profileId))
		{
			if (widget)
			{
				if (profileId == FCurProfileId)
					FWidgetLayout->removeWidget(widget->instance());
				widget->instance()->setParent(NULL);
				delete widget->instance();
			}
		}
		ui.cmbProfile->removeItem(ui.cmbProfile->findData(profileId.toString()));
		Options::node(OPV_DATASTREAMS_ROOT).removeChilds("settings-profile",profileId.toString());
	}
	FNewProfiles.clear();

	foreach(QUuid profileId, FDataManager->settingsProfiles())
	{
		if (ui.cmbProfile->findData(profileId.toString())<0)
			ui.cmbProfile->addItem(FDataManager->settingsProfileName(profileId), profileId.toString());

		foreach(IOptionsWidget *widget, FMethodWidgets.value(profileId))
			if (widget)
				widget->reset();
	}

	emit childReset();
}

void DataStreamsOptions::onAddProfileButtonClicked(bool)
{
	QString name = QInputDialog::getText(this,tr("Add Profile"),tr("Enter profile name:"));
	if (!name.isEmpty())
	{
		QUuid newProfileId = QUuid::createUuid().toString();
		FNewProfiles.append(newProfileId);
		ui.cmbProfile->addItem(name,newProfileId.toString());
		ui.cmbProfile->setCurrentIndex(ui.cmbProfile->count()-1);
		emit modified();
	}
}

void DataStreamsOptions::onDeleteProfileButtonClicked(bool)
{
	QMessageBox::StandardButton button = QMessageBox::warning(this,tr("Delete Profile"),tr("Do you really want to delete a current data streams profile?"),QMessageBox::Yes|QMessageBox::No);
	if (button == QMessageBox::Yes)
	{
		foreach(IOptionsWidget *widget, FMethodWidgets.take(FCurProfileId).values())
		{
			if (widget)
			{
				FWidgetLayout->removeWidget(widget->instance());
				widget->instance()->setParent(NULL);
				delete widget->instance();
			}
		}
		if (FNewProfiles.contains(FCurProfileId))
		{
			FNewProfiles.removeAll(FCurProfileId);
			Options::node(OPV_DATASTREAMS_ROOT).removeChilds("settings-profile",FCurProfileId.toString());
		}
		ui.cmbProfile->removeItem(ui.cmbProfile->currentIndex());
		emit modified();
	}
}

void DataStreamsOptions::onCurrentProfileChanged(int AIndex)
{
	foreach(IOptionsWidget *widget, FMethodWidgets.value(FCurProfileId))
	{
		FWidgetLayout->removeWidget(widget->instance());
		widget->instance()->setParent(NULL);
	}

	FCurProfileId = ui.cmbProfile->itemData(AIndex).toString();

	foreach(QString smethodNS, FDataManager->methods())
	{
		IOptionsWidget *widget = FMethodWidgets[FCurProfileId].value(smethodNS);
		if (!widget)
		{
			IDataStreamMethod *smethod = FDataManager->method(smethodNS);
			if (smethod)
			{
				widget = smethod->methodSettingsWidget(FDataManager->settingsProfileNode(FCurProfileId,smethodNS), false, ui.wdtSettings);
				FMethodWidgets[FCurProfileId].insert(smethodNS, widget);
				connect(widget->instance(),SIGNAL(modified()),SIGNAL(modified()));
				FCleanupHandler.add(widget->instance());
			}
		}
		if (widget)
		{
			FWidgetLayout->addWidget(widget->instance());
		}
	}

	if (!FCurProfileId.isNull())
	{
		ui.cmbProfile->setEditable(true);
		connect(ui.cmbProfile->lineEdit(),SIGNAL(editingFinished()),SLOT(onProfileEditingFinished()));
	}
	else if (ui.cmbProfile->lineEdit() != NULL)
	{
		disconnect(ui.cmbProfile->lineEdit(),SIGNAL(editingFinished()),this,SLOT(onProfileEditingFinished()));
		ui.cmbProfile->setEditable(false);
	}
	ui.pbtDeleteProfile->setEnabled(!FCurProfileId.isNull());
}

void DataStreamsOptions::onProfileEditingFinished()
{
	QString name = ui.cmbProfile->currentText();
	if (!name.isEmpty())
		ui.cmbProfile->setItemText(ui.cmbProfile->currentIndex(),name);
	emit modified();
}
