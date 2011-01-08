#include "messengeroptions.h"

#include <QKeyEvent>
#include <QFontDialog>

MessengerOptions::MessengerOptions(IMessageWidgets *AMessageWidgets, QWidget *AParent) : QWidget(AParent)
{
	ui.setupUi(this);
	FMessageWidgets = AMessageWidgets;
	connect(ui.spbEditorMinimumLines,SIGNAL(valueChanged(int)),SIGNAL(modified()));
	connect(ui.spbCleanChatTimeout,SIGNAL(valueChanged(int)),SIGNAL(modified()));
	reset();
}

MessengerOptions::~MessengerOptions()
{

}

void MessengerOptions::apply()
{
	Options::node(OPV_MESSAGES_EDITORMINIMUMLINES).setValue(ui.spbEditorMinimumLines->value());
	Options::node(OPV_MESSAGES_CLEANCHATTIMEOUT).setValue(ui.spbCleanChatTimeout->value());
	emit childApply();
}

void MessengerOptions::reset()
{
	ui.spbEditorMinimumLines->setValue(Options::node(OPV_MESSAGES_EDITORMINIMUMLINES).value().toInt());
	ui.spbCleanChatTimeout->setValue(Options::node(OPV_MESSAGES_CLEANCHATTIMEOUT).value().toInt());
	emit childReset();
}
