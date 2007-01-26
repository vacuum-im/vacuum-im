#ifndef ROSTERSVIEW_H
#define ROSTERSVIEW_H

#include "interfaces/irostersmodel.h"
#include "interfaces/irostersview.h"

class RostersView : 
  virtual public QTreeView,
  public IRostersView
{
  Q_OBJECT;
  Q_INTERFACES(IRostersView);

public:
  RostersView(IRostersModel *AModel, QWidget *AParent);
  ~RostersView();

  virtual QObject *instance() { return this; }

  //IRostersView
  virtual IRostersModel *rostersModel() const { return FRostersModel; }

private:
  IRostersModel *FRostersModel;    
};

#endif // ROSTERSVIEW_H
