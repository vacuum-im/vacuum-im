#include "optionsheader.h"

#include <QTextDocument>

OptionsHeader::OptionsHeader(const QString &ACaption, QWidget *AParent) : QLabel(AParent)
{
	setTextFormat(Qt::RichText);
	setText(QString("<h3>%1</h3>").arg(Qt::escape(ACaption)));
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
