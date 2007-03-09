#ifndef ROSTERSVIEW_H
#define ROSTERSVIEW_H

#include <QVector>
#include <QContextMenuEvent>
#include "../../interfaces/irostersview.h"
#include "../../interfaces/irostersmodel.h"

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
  virtual void addProxyModel(QAbstractProxyModel *AProxyModel);
  virtual QAbstractProxyModel *lastProxyModel() const { return FProxyModels.value(FProxyModels.count()-1,NULL); }
  virtual void removeProxyModel(QAbstractProxyModel *AProxyModel);
signals:
  virtual void proxyModelAdded(QAbstractProxyModel *);
  virtual void proxyModelRemoved(QAbstractProxyModel *);
  virtual void contextMenu(const QModelIndex &AIndex, Menu *AMenu);
protected:
  void drawBranches (QPainter *, const QRect &, const QModelIndex &) const {}
  void contextMenuEvent (QContextMenuEvent *AEvent);
private:
  IRostersModel *FRostersModel;
  QVector<QAbstractProxyModel *> FProxyModels;
  Menu *FContextMenu;
};

#endif // ROSTERSVIEW_H
