#ifndef ADIUMOPTIONSWIDGET_H
#define ADIUMOPTIONSWIDGET_H

#include <QWidget>
#include <interfaces/imessagestyles.h>
#include <interfaces/ioptionsmanager.h>
#include "adiummessagestyleplugin.h"
#include "ui_adiumoptionswidget.h"

class AdiumMessageStylePlugin;

class AdiumOptionsWidget : 
  public QWidget,
  public IOptionsWidget
{
  Q_OBJECT;
  Q_INTERFACES(IOptionsWidget);
public:
  AdiumOptionsWidget(AdiumMessageStylePlugin *APlugin, const OptionsNode &ANode, int AMessageType, QWidget *AParent = NULL);
  ~AdiumOptionsWidget();
  virtual QWidget *instance() { return this; }
public slots:
  virtual void apply(OptionsNode ANode);
  virtual void apply();
  virtual void reset();
signals:
  void modified();
  void childApply();
  void childReset();
public:
  IMessageStyleOptions styleOptions() const;
protected:
  void updateOptionsWidgets();
protected slots:
  void onStyleChanged(int AIndex);
  void onVariantChanged(int AIndex);
  void onSetFontClicked();
  void onDefaultFontClicked();
  void onImageLayoutChanged(int AIndex);
  void onBackgroundColorChanged(int AIndex);
  void onSetImageClicked();
  void onDefaultImageClicked();
private:
  Ui::AdiumOptionsWidgetClass ui;
private:
  AdiumMessageStylePlugin *FStylePlugin;
private:
  int FMessageType;
  OptionsNode FOptions;
  IMessageStyleOptions FStyleOptions;
};

#endif // ADIUMOPTIONSWIDGET_H
