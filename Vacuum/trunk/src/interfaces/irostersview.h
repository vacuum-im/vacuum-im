#ifndef IROSTERSVIEW_H
#define IROSTERSVIEW_H

#include <QTreeView>
#include <QAbstractProxyModel>
#include "../../interfaces/irostersmodel.h"
#include "../../utils/menu.h"


class IRostersView :
  virtual public QTreeView
{
public:
  virtual QObject *instance() = 0;
  virtual IRostersModel *rostersModel() const =0;
  virtual void addProxyModel(QAbstractProxyModel *AProxyModel) =0;
  virtual QAbstractProxyModel *lastProxyModel() const =0;
  virtual void removeProxyModel(QAbstractProxyModel *AProxyModel) =0;
signals:
  virtual void proxyModelAdded(QAbstractProxyModel *) =0;
  virtual void proxyModelRemoved(QAbstractProxyModel *) =0;
  virtual void contextMenu(const QModelIndex &, Menu *) =0;
};


class IRostersViewPlugin
{
public:
  virtual QObject *instance() = 0;
  virtual IRostersView *rostersView() =0;
};

Q_DECLARE_INTERFACE(IRostersView,"Vacuum.Plugin.IRostersView/1.0");
Q_DECLARE_INTERFACE(IRostersViewPlugin,"Vacuum.Plugin.IRostersViewPlugin/1.0");

#endif