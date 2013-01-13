#ifndef TOOLBARWIDGET_H
#define TOOLBARWIDGET_H

#include <QToolBar>
#include "../../interfaces/imessenger.h"
#include "../../utils/toolbarchanger.h"

class ToolBarWidget : 
  public IToolBarWidget
{
  Q_OBJECT;
  Q_INTERFACES(IToolBarWidget);
public:
  ToolBarWidget(IInfoWidget *AInfo, IViewWidget *AView, IEditWidget *AEdit, IReceiversWidget *AReceivers);
  ~ToolBarWidget();
  virtual ToolBarChanger *toolBarChanger() const { return FToolBarChanger; }
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
  ToolBarChanger *FToolBarChanger;    
};

#endif // TOOLBARWIDGET_H
