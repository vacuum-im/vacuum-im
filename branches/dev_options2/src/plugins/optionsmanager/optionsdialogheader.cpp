#include "optionsdialogheader.h"

#include <QTextDocument>

OptionsDialogHeader::OptionsDialogHeader(const QString &ACaption, QWidget *AParent) : QLabel(AParent)
{
	setTextFormat(Qt::RichText);
	setText(QString("<h3>%1</h3>").arg(Qt::escape(ACaption)));
}

OptionsDialogHeader::~OptionsDialogHeader()
{

}

void OptionsDialogHeader::apply()
{
	emit childApply();
}

void OptionsDialogHeader::reset()
{
	emit childReset();
}
