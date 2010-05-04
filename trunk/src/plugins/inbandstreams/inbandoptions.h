#ifndef INBANDOPTIONS_H
#define INBANDOPTIONS_H

#include <QWidget>
#include <interfaces/iinbandstreams.h>
#include <interfaces/ioptionsmanager.h>
#include "ui_inbandoptions.h"

class InBandOptions : 
  public QWidget,
  public IOptionsWidget
{
  Q_OBJECT;
  Q_INTERFACES(IOptionsWidget);
public:
  InBandOptions(IInBandStreams *AInBandStreams, IInBandStream *AStream, bool AReadOnly, QWidget *AParent);
  InBandOptions(IInBandStreams *AInBandStreams, const OptionsNode &ANode, bool AReadOnly, QWidget *AParent);
  ~InBandOptions();
  virtual QWidget *instance() { return this; }
public slots:
  void apply(OptionsNode ANode);
  void apply(IInBandStream *AStream);
  virtual void apply();
  virtual void reset();
signals:
  void modified();
  void childApply();
  void childReset();
protected:
  void initialize(bool AReadOnly);
protected slots:
  void onMaxBlockSizeValueChanged(int AValue);
private:
  Ui::InBandOptionsClass ui;
private:
  IInBandStreams *FInBandStreams;
private:
  OptionsNode FOptions;
  IInBandStream *FStream;
};

#endif // INBANDOPTIONS_H
