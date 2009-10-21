#ifndef SIMPLEOPTIONSWIDGET_H
#define SIMPLEOPTIONSWIDGET_H

#include <QWidget>
#include <interfaces/imessagestyles.h>
#include "simplemessagestyleplugin.h"
#include "ui_simpleoptionswidget.h"

class SimpleMessageStylePlugin;

class SimpleOptionsWidget : 
  public QWidget,
  public IMessageStyleSettings
{
  Q_OBJECT;
  Q_INTERFACES(IMessageStyleSettings);
public:
  SimpleOptionsWidget(SimpleMessageStylePlugin *APlugin, int AMessageType, const QString &AContext, QWidget *AParent = NULL);
  ~SimpleOptionsWidget();
  virtual QWidget *instance() { return this; }
  virtual int messageType() const;
  virtual QString context() const;
  virtual bool isModified(int AMessageType, const QString &AContext) const;
  virtual void setModified(bool AModified, int AMessageType, const QString &AContext);
  virtual IMessageStyleOptions styleOptions(int AMessageType, const QString &AContext) const;
  virtual void loadSettings(int AMessageType, const QString &AContext);
signals:
  virtual void settingsChanged();
protected:
  void startSignalTimer();
  void updateOptionsWidgets();
protected slots:
  void onStyleChanged(int AIndex);
  void onVariantChanged(int AIndex);
  void onSetFontClicked();
  void onDefaultFontClicked();
  void onBackgroundColorChanged(int AIndex);
  void onSetImageClicked();
  void onDefaultImageClicked();
  void onSettingsChanged();
private:
  Ui::SimpleOptionsWidgetClass ui;
private:
  SimpleMessageStylePlugin *FStylePlugin;
private:
  bool FModifyEnabled;
  bool FTimerStarted;
  int FActiveType;
  QString FActiveContext;
  QMap<int, QMap<QString, bool> > FModified;
  QMap<int, QMap<QString, IMessageStyleOptions> > FOptions;
};

#endif // SIMPLEOPTIONSWIDGET_H
