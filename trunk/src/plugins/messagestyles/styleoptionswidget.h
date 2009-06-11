#ifndef STYLEOPTIONSWIDGET_H
#define STYLEOPTIONSWIDGET_H

#include "ui_styleoptionswidget.h"
#include "../../interfaces/imessagestyles.h"
#include "../../interfaces/ipresence.h"
#include "../../interfaces/iroster.h"
#include "../../utils/message.h"

class StyleOptionsWidget : 
  public QWidget
{
  Q_OBJECT;
public:
  StyleOptionsWidget(IMessageStyles *AMessageStyles, QWidget *AParent = NULL);
  ~StyleOptionsWidget();
public slots:
  void apply();
signals:
  void optionsApplied();
protected:
  void updateActiveSettings();
  void createViewContent();
protected slots:
  void onStyleSettingsChanged();
  void onMessageTypeChanged(int AIndex);
  void onStyleEngineChanged(int AIndex);
private:
  Ui::StyleOptionsWidgetClass ui;
private:
  IMessageStyles *FMessageStyles;
private:
  QWidget *FActiveView;
  IMessageStyle *FActiveStyle;
  IMessageStylePlugin *FActivePlugin;
  IMessageStyleSettings *FActiveSettings;
  QMap<int, QString> FPluginForMessage;
  QMap<IMessageStylePlugin *, IMessageStyleSettings *> FSettings;
};

#endif // STYLEOPTIONSWIDGET_H
