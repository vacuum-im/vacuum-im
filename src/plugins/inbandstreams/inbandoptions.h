#ifndef INBANDOPTIONS_H
#define INBANDOPTIONS_H

#include <QWidget>
#include "../../interfaces/iinbandstreams.h"
#include "ui_inbandoptions.h"

class InBandOptions : 
  public QWidget
{
  Q_OBJECT;
public:
  InBandOptions(IInBandStreams *AInBandStreams, IInBandStream *AStream, bool AReadOnly, QWidget *AParent = NULL);
  InBandOptions(IInBandStreams *AInBandStreams, const QString &ASettingsNS, bool AReadOnly, QWidget *AParent = NULL);
  ~InBandOptions();
  void saveSettings(IInBandStream *AStream);
  void saveSettings(const QString &ASettingsNS);
public slots:
  void apply();
signals:
  void optionsAccepted();
protected:
  void initialize(bool AReadOnly);
protected slots:
  void onMaxBlockSizeValueChanged(int AValue);
private:
  Ui::InBandOptionsClass ui;
private:
  IInBandStreams *FInBandStreams;
private:
  QString FSettingsNS;
  IInBandStream *FStream;
};

#endif // INBANDOPTIONS_H
