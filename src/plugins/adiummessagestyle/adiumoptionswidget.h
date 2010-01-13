#ifndef ADIUMOPTIONSWIDGET_H
#define ADIUMOPTIONSWIDGET_H

#include <QWidget>
#include <interfaces/imessagestyles.h>
#include "adiummessagestyleplugin.h"
#include "ui_adiumoptionswidget.h"

class AdiumMessageStylePlugin;

class AdiumOptionsWidget : 
  public QWidget,
  public IMessageStyleSettings
{
  Q_OBJECT;
  Q_INTERFACES(IMessageStyleSettings);
public:
  AdiumOptionsWidget(AdiumMessageStylePlugin *APlugin, int AMessageType, const QString &AContext, QWidget *AParent = NULL);
  ~AdiumOptionsWidget();
  virtual QWidget *instance() { return this; }
  virtual int messageType() const;
  virtual QString context() const;
  virtual bool isModified(int AMessageType, const QString &AContext) const;
  virtual void setModified(bool AModified, int AMessageType, const QString &AContext);
  virtual IMessageStyleOptions styleOptions(int AMessageType, const QString &AContext) const;
  virtual void loadSettings(int AMessageType, const QString &AContext);
signals:
  void settingsChanged();
protected:
  void startSignalTimer();
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
  void onSettingsChanged();
private:
  Ui::AdiumOptionsWidgetClass ui;
private:
  AdiumMessageStylePlugin *FStylePlugin;
private:
  bool FModifyEnabled;
  bool FTimerStarted;
  int FActiveType;
  QString FActiveContext;
  QMap<int, QMap<QString, bool> > FModified;
  QMap<int, QMap<QString, IMessageStyleOptions> > FOptions;
};

#endif // ADIUMOPTIONSWIDGET_H
