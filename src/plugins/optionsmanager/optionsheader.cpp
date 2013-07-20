#include "optionsheader.h"

OptionsHeader::OptionsHeader(const QString &ACaption, QWidget *AParent) : QLabel(AParent)
{
	setTextFormat(Qt::RichText);
	setText(QString("<h3>%1</h3>").arg(ACaption.toHtmlEscaped()));
}

OptionsHeader::~OptionsHeader()
{

}

void OptionsHeader::apply()
{
	emit childApply();
}

void OptionsHeader::reset()
{
	emit childReset();
}
