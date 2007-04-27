#ifndef IROSTERSVIEW_H
#define IROSTERSVIEW_H

#include <QTreeView>
#include <QAbstractProxyModel>
#include "../../interfaces/irostersmodel.h"
#include "../../utils/menu.h"

#define ROSTERSVIEW_UUID "{BDD12B32-9C88-4e3c-9B36-2DCB5075288F}"

class IRostersView :
  virtual public QTreeView
{
public:
  virtual QObject *instance() = 0;
  virtual void setModel(IRostersModel *) =0; 
  virtual IRostersModel *rostersModel() const =0;
  virtual IRosterIndexDataHolder *defaultDataHolder() const =0;
  virtual void addProxyModel(QAbstractProxyModel *AProxyModel) =0;
  virtual QAbstractProxyModel *lastProxyModel() const =0;
  virtual void removeProxyModel(QAbstractProxyModel *AProxyModel) =0;
signals:
  virtual void modelAboutToBeSeted(IRostersModel *) =0;
  virtual void modelSeted(IRostersModel *) =0;
  virtual void proxyModelAboutToBeAdded(QAbstractProxyModel *) =0;
  virtual void proxyModelAdded(QAbstractProxyModel *) =0;
  virtual void proxyModelAboutToBeRemoved(QAbstractProxyModel *) =0;
  virtual void proxyModelRemoved(QAbstractProxyModel *) =0;
signals:
  virtual void contextMenu(const QModelIndex &, Menu *) =0;
  virtual void toolTipMap(const QModelIndex &, QMultiMap<int,QString> &AToolTips) =0;
};


class IRostersViewPlugin
{
public:
  virtual QObject *instance() = 0;
  virtual IRostersView *rostersView() =0;
  virtual bool showOfflineContacts() const =0;
public slots:
  virtual void setShowOfflineContacts(bool AShow) =0;
signals:
  virtual void viewCreated(IRostersView *) =0;
  virtual void viewDestroyed(IRostersView *) =0;
  virtual void showOfflineContactsChanged(bool) =0;
};

Q_DECLARE_INTERFACE(IRostersView,"Vacuum.Plugin.IRostersView/1.0");
Q_DECLARE_INTERFACE(IRostersViewPlugin,"Vacuum.Plugin.IRostersViewPlugin/1.0");

#endif