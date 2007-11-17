#ifndef SELECTICONMENU_H
#define SELECTICONMENU_H

#include <QShowEvent>
#include <QHideEvent>
#include <QVBoxLayout>
#include "../../utils/menu.h"
#include "../../utils/skin.h"
#include "selecticonwidget.h"

class SelectIconMenu : 
  private Menu
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
  QString iconset() const { return FIconset; }
  void setIconset(const QString &AFileName);
signals:
  void iconSelected(const QString &AIconsetFile, const QString &AIconFile);
public:
  virtual QSize sizeHint() const;
protected:
  void createWidget();
  void destroyWidget();
protected:
  virtual void hideEvent(QHideEvent *AEvent);
protected slots:
  void onAboutToShow();
  void onSkinIconsetChanged();
private:
  QSize FSizeHint;
  QVBoxLayout *FLayout;    
  QString FIconset;
  SelectIconWidget * FWidget;
};

#endif // SELECTICONMENU_H
