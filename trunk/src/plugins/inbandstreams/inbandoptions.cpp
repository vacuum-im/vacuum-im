#include "inbandoptions.h"

InBandOptions::InBandOptions(IInBandStreams *AInBandStreams, IInBandStream *AStream, bool AReadOnly, QWidget *AParent) : QWidget(AParent)
{
  FStream = AStream;
  FInBandStreams = AInBandStreams;

  initialize(AReadOnly);

  ui.spbMaxBlockSize->setValue(AStream->maximumBlockSize());
  ui.spbBlockSize->setValue(AStream->blockSize());
  ui.spbBlockSize->setMaximum(ui.spbMaxBlockSize->value());
  ui.cmbStanzaType->setCurrentIndex(ui.cmbStanzaType->findData(AStream->dataStanzaType()));
}

InBandOptions::InBandOptions(IInBandStreams *AInBandStreams, const QString &ASettingsNS, bool AReadOnly, QWidget *AParent) : QWidget(AParent)
{
  FStream = NULL;
  FSettingsNS = ASettingsNS;
  FInBandStreams = AInBandStreams;

  initialize(AReadOnly);
 
  ui.spbMaxBlockSize->setValue(AInBandStreams->maximumBlockSize(ASettingsNS));
  ui.spbBlockSize->setValue(AInBandStreams->blockSize(ASettingsNS));
  ui.spbBlockSize->setMaximum(ui.spbMaxBlockSize->value());
  ui.cmbStanzaType->setCurrentIndex(ui.cmbStanzaType->findData(FInBandStreams->dataStanzaType(ASettingsNS)));
}

InBandOptions::~InBandOptions()
{

}

void InBandOptions::saveSettings(const QString &ASettingsNS)
{
  FInBandStreams->setMaximumBlockSize(ASettingsNS, ui.spbMaxBlockSize->value());
  FInBandStreams->setBlockSize(ASettingsNS, ui.spbBlockSize->value());
  FInBandStreams->setDataStanzaType(ASettingsNS, ui.cmbStanzaType->itemData(ui.cmbStanzaType->currentIndex()).toInt());
}

void InBandOptions::saveSettings(IInBandStream *AStream)
{
  AStream->setMaximumBlockSize(ui.spbMaxBlockSize->value());
  AStream->setBlockSize(ui.spbBlockSize->value());
  AStream->setDataStanzaType(ui.cmbStanzaType->itemData(ui.cmbStanzaType->currentIndex()).toInt());
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

  connect(ui.spbMaxBlockSize,SIGNAL(valueChanged(int)),SLOT(onMaxBlockSizeValueChanged(int)));
}

void InBandOptions::onMaxBlockSizeValueChanged(int AValue)
{
  ui.spbBlockSize->setMaximum(AValue);
}

void InBandOptions::apply()
{
  if (FStream)
    saveSettings(FStream);
  else
    saveSettings(FSettingsNS);
  emit optionsAccepted();
}