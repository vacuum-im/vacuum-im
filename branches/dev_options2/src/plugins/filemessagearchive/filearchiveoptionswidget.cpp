#include "filearchiveoptionswidget.h"

#include <QFileDialog>
#include <definitions/optionvalues.h>
#include <utils/options.h>

FileArchiveOptionsWidget::FileArchiveOptionsWidget(IPluginManager *APluginManager, QWidget *AParent) : QWidget(AParent)
{
	ui.setupUi(this);
	FPluginManager = APluginManager;

	ui.lneLocation->setEnabled(ui.chbLocation->isChecked());
	ui.tlbLocation->setEnabled(ui.chbLocation->isChecked());

	connect(ui.chbLocation,SIGNAL(toggled(bool)),SIGNAL(modified()));
	connect(ui.lneLocation,SIGNAL(textChanged(const QString &)),SIGNAL(modified()));
	connect(ui.chbDatabaseSync,SIGNAL(toggled(bool)),SIGNAL(modified()));

	connect(ui.tlbLocation,SIGNAL(clicked()),SLOT(onSelectLocationFolder()));
	connect(ui.chbLocation,SIGNAL(toggled(bool)),ui.lneLocation,SLOT(setEnabled(bool)));
	connect(ui.chbLocation,SIGNAL(toggled(bool)),ui.tlbLocation,SLOT(setEnabled(bool)));

	reset();
}

FileArchiveOptionsWidget::~FileArchiveOptionsWidget()
{

}

void FileArchiveOptionsWidget::apply()
{
	Options::node(OPV_FILEARCHIVE_HOMEPATH).setValue(ui.chbLocation->isChecked() ? ui.lneLocation->text() : QString(""));
	Options::node(OPV_FILEARCHIVE_DATABASESYNC).setValue(ui.chbDatabaseSync->isChecked());
	emit childApply();
}

void FileArchiveOptionsWidget::reset()
{
	QString path = Options::node(OPV_FILEARCHIVE_HOMEPATH).value().toString();
	ui.chbLocation->setChecked(!path.isEmpty());
	ui.lneLocation->setText(!path.isEmpty() ? path : FPluginManager->homePath());
	ui.chbDatabaseSync->setChecked(Options::node(OPV_FILEARCHIVE_DATABASESYNC).value().toBool());
	emit childReset();
}

void FileArchiveOptionsWidget::onSelectLocationFolder()
{
	QString path = QFileDialog::getExistingDirectory(this,tr("Select the location for the file archive"));
	if (!path.isEmpty())
		ui.lneLocation->setText(path);
}
