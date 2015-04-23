#include "filestreamsoptionswidget.h"

#include <QVBoxLayout>
#include <QFileDialog>
#include <definitions/optionvalues.h>
#include <utils/options.h>

FileStreamsOptionsWidget::FileStreamsOptionsWidget(IFileStreamsManager *AFileManager, QWidget *AParent) : QWidget(AParent)
{
	ui.setupUi(this);
	FFileManager = AFileManager;

	connect(ui.tlbDirectory, SIGNAL(clicked()), SLOT(onDirectoryButtonClicked()));
	connect(ui.lneDirectory,SIGNAL(textChanged(const QString &)),SIGNAL(modified()));

	reset();
}

void FileStreamsOptionsWidget::apply()
{
	Options::node(OPV_FILESTREAMS_DEFAULTDIR).setValue(ui.lneDirectory->text());
	emit childApply();
}

void FileStreamsOptionsWidget::reset()
{
	ui.lneDirectory->setText(Options::node(OPV_FILESTREAMS_DEFAULTDIR).value().toString());
	emit childReset();
}

void FileStreamsOptionsWidget::onDirectoryButtonClicked()
{
	QString dir = QFileDialog::getExistingDirectory(this,tr("Select default directory"), ui.lneDirectory->text());
	if (!dir.isEmpty())
		ui.lneDirectory->setText(dir);
}
