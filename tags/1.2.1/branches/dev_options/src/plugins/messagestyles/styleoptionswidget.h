#ifndef STYLEOPTIONSWIDGET_H
#define STYLEOPTIONSWIDGET_H

#include <definations/optionvalues.h>
#include <interfaces/imessagestyles.h>
#include <interfaces/ioptionsmanager.h>
#include <interfaces/ipresence.h>
#include <interfaces/iroster.h>
#include <utils/message.h>
#include <utils/options.h>
#include "ui_styleoptionswidget.h"

class StyleOptionsWidget : 
  public QWidget,
  public IOptionsWidget
{
  Q_OBJECT;
  Q_INTERFACES(IOptionsWidget);
public:
  StyleOptionsWidget(IMessageStyles *AMessageStyles, QWidget *AParent);
  ~StyleOptionsWidget();
  virtual QWidget *instance() { return this; }
public slots:
  void apply();
  void reset();
signals:
  void modified();
  void childApply();
  void childReset();
public slots:
  void startStyleViewUpdate();
protected:
  void createViewContent();
  QWidget *updateActiveSettings();
protected slots:
  void onUpdateStyleView();
  void onMessageTypeChanged(int AIndex);
  void onStyleEngineChanged(int AIndex);
private:
  Ui::StyleOptionsWidgetClass ui;
private:
  IMessageStyles *FMessageStyles;
private:
  bool FUpdateStarted;
  QWidget *FActiveView;
  IMessageStyle *FActiveStyle;
  IOptionsWidget *FActiveSettings;
  QMap<int, QString> FMessagePlugin;
  QMap<int, IOptionsWidget *> FMessageWidget;
};

#endif // STYLEOPTIONSWIDGET_H
