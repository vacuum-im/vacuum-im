#ifndef ROSTERSVIEW_H
#define ROSTERSVIEW_H

#include <QContextMenuEvent>
#include <QPainter>
#include "../../interfaces/irostersview.h"
#include "../../interfaces/irostersmodel.h"
#include "../../interfaces/isettings.h"
#include "indexdataholder.h"

class RostersView : 
  virtual public QTreeView,
  public IRostersView
{
  Q_OBJECT;
  Q_INTERFACES(IRostersView);

public:
  RostersView(QWidget *AParent = NULL);
  ~RostersView();

  virtual QObject *instance() { return this; }

  //IRostersView
  virtual void setModel(IRostersModel *AModel); 
  virtual IRostersModel *rostersModel() const { return FRostersModel; }
  virtual IRosterIndexDataHolder *defaultDataHolder() const { return FIndexDataHolder; }
  virtual void addProxyModel(QAbstractProxyModel *AProxyModel);
  virtual QAbstractProxyModel *lastProxyModel() const { return FProxyModels.value(FProxyModels.count()-1,NULL); }
  virtual void removeProxyModel(QAbstractProxyModel *AProxyModel);
  virtual QModelIndex mapToModel(const QModelIndex &AProxyIndex);
  virtual QModelIndex mapFromModel(const QModelIndex &AModelIndex);
signals:
  virtual void modelAboutToBeSeted(IRostersModel *);
  virtual void modelSeted(IRostersModel *);
  virtual void proxyModelAboutToBeAdded(QAbstractProxyModel *);
  virtual void proxyModelAdded(QAbstractProxyModel *);
  virtual void proxyModelAboutToBeRemoved(QAbstractProxyModel *);
  virtual void proxyModelRemoved(QAbstractProxyModel *);
signals:
  virtual void contextMenu(const QModelIndex &AIndex, Menu *AMenu);
  virtual void toolTipMap(const QModelIndex &AIndex, QMultiMap<int,QString> &AToolTips);
protected:
  void drawBranches(QPainter *APainter, const QRect &ARect, const QModelIndex &AIndex) const;
  void contextMenuEvent(QContextMenuEvent *AEvent);
  //QAbstractItemView
  virtual bool viewportEvent(QEvent *AEvent);
protected slots:
  void onIndexCreated(IRosterIndex *AIndex, IRosterIndex *AParent);
private:
  IRostersModel *FRostersModel;
private:
  QList<QAbstractProxyModel *> FProxyModels;
  Menu *FContextMenu;
  IndexDataHolder *FIndexDataHolder;
};

#endif // ROSTERSVIEW_H
