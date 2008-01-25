#ifndef ROSTERSVIEW_H
#define ROSTERSVIEW_H

#include <QContextMenuEvent>
#include <QPainter>
#include <QTimer>
#include "../../definations/tooltiporders.h"
#include "../../definations/rosterlabelorders.h"
#include "../../definations/rosterindextyperole.h"
#include "../../interfaces/irostersview.h"
#include "../../interfaces/irostersmodel.h"
#include "../../interfaces/isettings.h"
#include "rosterindexdelegate.h" 


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
  virtual bool repaintRosterIndex(IRosterIndex *AIndex);
  virtual void expandIndexParents(IRosterIndex *AIndex);
  virtual void expandIndexParents(const QModelIndex &AIndex);
  //--ProxyModels
  virtual void addProxyModel(QAbstractProxyModel *AProxyModel);
  virtual QList<QAbstractProxyModel *> proxyModels() const { return FProxyModels; }
  virtual QAbstractProxyModel *lastProxyModel() const { return FProxyModels.value(FProxyModels.count()-1,NULL); }
  virtual void removeProxyModel(QAbstractProxyModel *AProxyModel);
  virtual QModelIndex mapToModel(const QModelIndex &AProxyIndex) const;
  virtual QModelIndex mapFromModel(const QModelIndex &AModelIndex) const;
  virtual QModelIndex mapToProxy(QAbstractProxyModel *AProxyModel, const QModelIndex &AModelIndex) const;
  virtual QModelIndex mapFromProxy(QAbstractProxyModel *AProxyModel, const QModelIndex &AProxyIndex) const;
  //--IndexLabel
  virtual int createIndexLabel(int AOrder, const QVariant &ALabel, int AFlags = 0);
  virtual void updateIndexLabel(int ALabelId, const QVariant &ALabel, int AFlags = 0);
  virtual void insertIndexLabel(int ALabelId, IRosterIndex *AIndex);
  virtual void removeIndexLabel(int ALabelId, IRosterIndex *AIndex);
  virtual void destroyIndexLabel(int ALabelId);
  virtual int labelAt(const QPoint &APoint, const QModelIndex &AIndex) const;
  virtual QRect labelRect(int ALabeld, const QModelIndex &AIndex) const;
  //--IndexNotify
  virtual int appendNotify(IRosterIndexList AIndexes, int AOrder, const QIcon &AIcon, const QString &AToolTip, int AFlags=0);
  virtual QList<int> indexNotifies(IRosterIndex *Index, int AOrder) const;
  virtual void updateNotify(int ANotifyId, const QIcon &AIcon, const QString &AToolTip, int AFlags=0);
  virtual void removeNotify(int ANotifyId);
  //--ClickHookers
  virtual void insertClickHooker(int AOrder, IRostersClickHooker *AHooker);
  virtual void removeClickHooker(int AOrder, IRostersClickHooker *AHooker);
  //--FooterText
  virtual void insertFooterText(int AOrderAndId, const QString &AText, IRosterIndex *AIndex);
  virtual void removeFooterText(int AOrderAndId, IRosterIndex *AIndex);
signals:
  virtual void modelAboutToBeSeted(IRostersModel *AModel);
  virtual void modelSeted(IRostersModel *AModel);
  virtual void proxyModelAboutToBeAdded(QAbstractProxyModel *AProxyModel);
  virtual void proxyModelAdded(QAbstractProxyModel *AProxyModel);
  virtual void proxyModelAboutToBeRemoved(QAbstractProxyModel *AProxyModel);
  virtual void proxyModelRemoved(QAbstractProxyModel *AProxyModel);
  virtual void lastModelAboutToBeChanged(QAbstractItemModel *AModel);
  virtual void lastModelChanged(QAbstractItemModel *AModel);
  virtual void contextMenu(IRosterIndex *AIndex, Menu *AMenu);
  virtual void labelContextMenu(IRosterIndex *AIndex, int ALabelId, Menu *AMenu);
  virtual void labelToolTips(IRosterIndex *AIndex, int ALabelId, QMultiMap<int,QString> &AToolTips);
  virtual void labelClicked(IRosterIndex *AIndex, int ALabelId);
  virtual void labelDoubleClicked(IRosterIndex *AIndex, int ALabelId, bool &AAccepted);
  virtual void notifyContextMenu(IRosterIndex *AIndex, int ANotifyId, Menu *AMenu);
  virtual void notifyActivated(IRosterIndex *AIndex, int ANotifyId);
  virtual void notifyRemovedByIndex(IRosterIndex *AIndex, int ANotifyId);
signals:
  virtual void indexAboutToBeRemoved(const QModelIndex &AParent, int AStart, int AEnd);
  virtual void indexInserted(const QModelIndex &AParent, int AStart, int AEnd);
public:
  bool checkOption(IRostersView::Option AOption) const;
  void setOption(IRostersView::Option AOption, bool AValue);
protected:
  QStyleOptionViewItemV2 indexOption(const QModelIndex &AIndex) const;
  void appendBlinkLabel(int ALabelId);
  void removeBlinkLabel(int ALabelId);
  QString intId2StringId(int AIntId);
  void removeLabels();
  void setLastModel(QAbstractItemModel *AModel);
protected:
  //QTreeView
  virtual void drawBranches(QPainter *APainter, const QRect &ARect, const QModelIndex &AIndex) const;
  virtual void rowsAboutToBeRemoved(const QModelIndex &AParent, int AStart, int AEnd);
  virtual void rowsInserted(const QModelIndex &AParent, int AStart, int AEnd);
  //QAbstractItemView
  virtual bool viewportEvent(QEvent *AEvent);
  //QWidget
  virtual void contextMenuEvent(QContextMenuEvent *AEvent);
  virtual void mouseDoubleClickEvent(QMouseEvent *AEvent);
  virtual void mousePressEvent(QMouseEvent *AEvent);
  virtual void mouseReleaseEvent(QMouseEvent *AEvent);
protected slots:
  void onIndexDestroyed(IRosterIndex *AIndex);
  void onBlinkTimer();
private:
  IRostersModel *FRostersModel;
private:
  Menu *FContextMenu;
private:
  int FLabelId;
  QTimer FBlinkTimer;
  QSet<int> FBlinkLabels;
  bool FBlinkShow;
  int FPressedLabel;
  IRosterIndex *FPressedIndex;
  QHash<int, QVariant> FIndexLabels;
  QHash<int, int /*Order*/> FIndexLabelOrders;
  QHash<int, int /*Flags*/> FIndexLabelFlags;
  QHash<int, QSet<IRosterIndex *> > FIndexLabelIndexes;
private:
  int FNotifyId;
  struct NotifyItem
  {
    int notifyId;
    int order;
    QIcon icon;
    QString toolTip;
    int flags;
    IRosterIndexList indexes;
  };
  QHash<int /*id*/ ,NotifyItem> FNotifyItems;
  QHash<int /*label*/, QList<int> > FNotifyLabelItems;
  QHash<IRosterIndex *, QHash<int /*order*/, int /*labelid*/> > FNotifyIndexOrderLabel;
private:
  QMultiMap<int,IRostersClickHooker *> FClickHookers;
private:
  int FOptions;
  RosterIndexDelegate *FRosterIndexDelegate;
  QList<QAbstractProxyModel *> FProxyModels;
};

#endif // ROSTERSVIEW_H
