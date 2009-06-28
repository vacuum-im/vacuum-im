#ifndef MENUBARWIDGET_H
#define MENUBARWIDGET_H

#include <QMenuBar>
#include "../../interfaces/imessagewidgets.h"

class MenuBarWidget : 
  public QMenuBar,
  public IMenuBarWidget
{
  Q_OBJECT;
  Q_INTERFACES(IMenuBarWidget);
public:
  MenuBarWidget(IInfoWidget *AInfo, IViewWidget *AView, IEditWidget *AEdit, IReceiversWidget *AReceivers);
  ~MenuBarWidget();
  virtual QMenuBar *instance() { return this; }
  virtual MenuBarChanger *menuBarChanger() const { return FMenuBarChanger; }
  virtual IInfoWidget *infoWidget() const { return FInfoWidget; }
  virtual IViewWidget *viewWidget() const { return FViewWidget; }
  virtual IEditWidget *editWidget() const { return FEditWidget; }
  virtual IReceiversWidget *receiversWidget() const { return FReceiversWidget; }
private:
  IInfoWidget *FInfoWidget;
  IViewWidget *FViewWidget;
  IEditWidget *FEditWidget;
  IReceiversWidget *FReceiversWidget;
private:
  MenuBarChanger *FMenuBarChanger;
};

#endif // MENUBARWIDGET_H
