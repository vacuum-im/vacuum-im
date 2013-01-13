#include "inbandoptions.h"

InBandOptions::InBandOptions(IInBandStreams *AInBandStreams, IInBandStream *AStream, bool AReadOnly, QWidget *AParent) : QWidget(AParent)
{
	FStream = AStream;
	FInBandStreams = AInBandStreams;
	initialize(AReadOnly);
	reset();
}

InBandOptions::InBandOptions(IInBandStreams *AInBandStreams, const OptionsNode &ANode, bool AReadOnly, QWidget *AParent) : QWidget(AParent)
{
	FStream = NULL;
	FOptions = ANode;
	FInBandStreams = AInBandStreams;
	initialize(AReadOnly);
	reset();
}

InBandOptions::~InBandOptions()
{

}

void InBandOptions::apply(OptionsNode ANode)
{
	OptionsNode node = ANode.isNull() ? FOptions : ANode;
	node.setValue(ui.spbMaxBlockSize->value(),"max-block-size");
	node.setValue(ui.spbBlockSize->value(),"block-size");
	node.setValue(ui.cmbStanzaType->itemData(ui.cmbStanzaType->currentIndex()).toInt(),"stanza-type");
	emit childApply();
}

void InBandOptions::apply(IInBandStream *AStream)
{
	AStream->setMaximumBlockSize(ui.spbMaxBlockSize->value());
	AStream->setBlockSize(ui.spbBlockSize->value());
	AStream->setDataStanzaType(ui.cmbStanzaType->itemData(ui.cmbStanzaType->currentIndex()).toInt());
	emit childApply();
}

void InBandOptions::apply()
{
	if (FStream)
		apply(FStream);
	else
		apply(FOptions);
}

void InBandOptions::reset()
{
	if (FStream)
	{
		ui.spbMaxBlockSize->setValue(FStream->maximumBlockSize());
		ui.spbBlockSize->setValue(FStream->blockSize());
		ui.spbBlockSize->setMaximum(ui.spbMaxBlockSize->value());
		ui.cmbStanzaType->setCurrentIndex(ui.cmbStanzaType->findData(FStream->dataStanzaType()));
	}
	else
	{
		ui.spbMaxBlockSize->setValue(FOptions.value("max-block-size").toInt());
		ui.spbBlockSize->setValue(FOptions.value("block-size").toInt());
		ui.spbBlockSize->setMaximum(ui.spbMaxBlockSize->value());
		ui.cmbStanzaType->setCurrentIndex(ui.cmbStanzaType->findData(FOptions.value("stanza-type").toInt()));
	}
	emit childReset();
}

void InBandOptions::initialize(bool AReadOnly)
{
	ui.setupUi(this);
	ui.grbSettings->setTitle(FInBandStreams->methodName());

	ui.cmbStanzaType->addItem(tr("Packet-Query (recommended)"), IInBandStream::StanzaIq);
	ui.cmbStanzaType->addItem(tr("Packet-Message"), IInBandStream::StanzaMessage);

	ui.spbBlockSize->setReadOnly(AReadOnly);
	ui.spbMaxBlockSize->setReadOnly(AReadOnly);
	ui.cmbStanzaType->setEnabled(!AReadOnly);

	connect(ui.spbBlockSize,SIGNAL(valueChanged(int)),SIGNAL(modified()));
	connect(ui.spbMaxBlockSize,SIGNAL(valueChanged(int)),SIGNAL(modified()));
	connect(ui.cmbStanzaType,SIGNAL(currentIndexChanged(int)),SIGNAL(modified()));
	connect(ui.spbMaxBlockSize,SIGNAL(valueChanged(int)),SLOT(onMaxBlockSizeValueChanged(int)));
}

void InBandOptions::onMaxBlockSizeValueChanged(int AValue)
{
	ui.spbBlockSize->setMaximum(AValue);
}
