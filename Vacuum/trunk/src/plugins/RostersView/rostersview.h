#ifndef ROSTERSVIEW_H
#define ROSTERSVIEW_H

#include <QContextMenuEvent>
#include <QPainter>
#include "../../interfaces/irostersview.h"
#include "../../interfaces/irostersmodel.h"
#include "sortfilterproxymodel.h"

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
  virtual QModelIndex mapToModel(const QModelIndex &AProxyIndex);
  virtual QModelIndex mapFromModel(const QModelIndex &AModelIndex);
public slots:
  virtual bool showOfflineContacts() const;
  virtual void setShowOfflineContacts(bool AShow);
signals:
  virtual void proxyModelAdded(QAbstractProxyModel *);
  virtual void proxyModelRemoved(QAbstractProxyModel *);
  virtual void contextMenu(const QModelIndex &AIndex, Menu *AMenu);
  virtual void showOfflineContactsChanged(bool AShow);
protected:
  void drawBranches (QPainter *APainter, const QRect &ARect, const QModelIndex &AIndex) const;
  void contextMenuEvent (QContextMenuEvent *AEvent);
protected slots:
  void onModelStreamAdded(const Jid &AStreamJid);
  void onModelStreamRemoved(const Jid &AStreamJid);
private:
  IRostersModel *FRostersModel;
  QList<QAbstractProxyModel *> FProxyModels;
  Menu *FContextMenu;
  SortFilterProxyModel *FSortFilterProxyModel;
};

#endif // ROSTERSVIEW_H
