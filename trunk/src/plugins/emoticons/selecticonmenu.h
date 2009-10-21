#ifndef SELECTICONMENU_H
#define SELECTICONMENU_H

#include <QShowEvent>
#include <QHideEvent>
#include <QVBoxLayout>
#include <definations/resources.h>
#include <utils/menu.h>
#include "selecticonwidget.h"

class SelectIconMenu : 
  public Menu
{
  Q_OBJECT;
public:
  SelectIconMenu(QWidget *AParent = NULL);
  ~SelectIconMenu();
  //Menu
  Action *menuAction();
  void setIcon(const QIcon &AIcon);
  void setTitle(const QString &ATitle);
  //SelectIconMenu
  QWidget *instance() { return this; }
  QString iconset() const;
  void setIconset(const QString &ASubStorage);
signals:
  void iconSelected(const QString &ASubStorage, const QString &AIconKey);
public:
  virtual QSize sizeHint() const;
protected:
  void createWidget();
  void destroyWidget();
protected:
  virtual void hideEvent(QHideEvent *AEvent);
protected slots:
  void onAboutToShow();
private:
  QSize FSizeHint;
  QVBoxLayout *FLayout;    
  IconStorage *FStorage;
  SelectIconWidget *FWidget;
};

#endif // SELECTICONMENU_H
