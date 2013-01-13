#ifndef SELECTICONMENU_H
#define SELECTICONMENU_H

#include <QVBoxLayout>
#include <definations/resources.h>
#include <utils/menu.h>
#include <utils/iconstorage.h>
#include "selecticonwidget.h"

class SelectIconMenu : 
  public Menu
{
  Q_OBJECT;
public:
  SelectIconMenu(const QString &AIconset, QWidget *AParent = NULL);
  ~SelectIconMenu();
  QWidget *instance() { return this; }
  QString iconset() const;
  void setIconset(const QString &ASubStorage);
signals:
  void iconSelected(const QString &ASubStorage, const QString &AIconKey);
public:
  virtual QSize sizeHint() const;
protected slots:
  void onAboutToShow();
private:
  QVBoxLayout *FLayout;    
  IconStorage *FStorage;
};

#endif // SELECTICONMENU_H
