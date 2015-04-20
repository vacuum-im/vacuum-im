#include "inbandoptionswidget.h"

InBandOptionsWidget::InBandOptionsWidget(IInBandStreams *AInBandStreams, const OptionsNode &ANode, QWidget *AParent) : QWidget(AParent)
{
	ui.setupUi(this);

	FOptionsNode = ANode;
	FInBandStreams = AInBandStreams;

	connect(ui.spbBlockSize,SIGNAL(valueChanged(int)),SIGNAL(modified()));
	connect(ui.chbDontWaitReceipt,SIGNAL(stateChanged(int)),SIGNAL(modified()));

	reset();
}

void InBandOptionsWidget::apply()
{
	FOptionsNode.setValue(ui.spbBlockSize->value(),"block-size");
	FOptionsNode.setValue(ui.chbDontWaitReceipt->isChecked() ? IInBandStream::StanzaMessage : IInBandStream::StanzaIq,"stanza-type");
	emit childApply();
}

void InBandOptionsWidget::reset()
{
	ui.spbBlockSize->setValue(FOptionsNode.value("block-size").toInt());
	ui.chbDontWaitReceipt->setChecked(FOptionsNode.value("stanza-type").toInt() != IInBandStream::StanzaIq);
	emit childReset();
}
