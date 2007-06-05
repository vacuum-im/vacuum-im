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
  enum LabelFlags {
    LabelBlink        =0x1
  };
public:
  virtual QObject *instance() = 0;
  virtual void setModel(IRostersModel *AModel) =0; 
  virtual IRostersModel *rostersModel() const =0;
  virtual IRosterIndexDataHolder *defaultDataHolder() const =0;
  virtual void addProxyModel(QAbstractProxyModel *AProxyModel) =0;
  virtual QAbstractProxyModel *lastProxyModel() const =0;
  virtual void removeProxyModel(QAbstractProxyModel *AProxyModel) =0;
  virtual int createIndexLabel(int AOrder, const QVariant &ALabel, int AFlags = 0) =0;
  virtual void updateIndexLabel(int ALabelId, const QVariant &ALabel, int AFlags = 0) =0;
  virtual void insertIndexLabel(int ALabelId, IRosterIndex *AIndex) =0;
  virtual void removeIndexLabel(int ALabelId, IRosterIndex *AIndex) =0;
  virtual void destroyIndexLabel(int ALabelId) =0;
  virtual int labelAt(const QPoint &APoint, const QModelIndex &AIndex) const =0;
  virtual QRect labelRect(int ALabeld, const QModelIndex &AIndex) const =0;
signals:
  virtual void modelAboutToBeSeted(IRostersModel *) =0;
  virtual void modelSeted(IRostersModel *) =0;
  virtual void proxyModelAboutToBeAdded(QAbstractProxyModel *) =0;
  virtual void proxyModelAdded(QAbstractProxyModel *) =0;
  virtual void proxyModelAboutToBeRemoved(QAbstractProxyModel *) =0;
  virtual void proxyModelRemoved(QAbstractProxyModel *) =0;
  virtual void contextMenu(const QModelIndex &, Menu *) =0;
  virtual void toolTips(const QModelIndex &, QMultiMap<int,QString> &AToolTips) =0;
  virtual void labelContextMenu(const QModelIndex &, int ALabelId, Menu *) =0;
  virtual void labelToolTips(const QModelIndex &, int ALabelId, QMultiMap<int,QString> &AToolTips) =0;
  virtual void labelClicked(const QModelIndex &, int ALabelId) =0;
  virtual void labelDoubleClicked(const QModelIndex &, int ALabelId, bool &AAccepted) =0;
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