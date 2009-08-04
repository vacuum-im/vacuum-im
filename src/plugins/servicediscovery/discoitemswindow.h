#ifndef DISCOITEMSWINDOW_H
#define DISCOITEMSWINDOW_H

#include <QTimer>
#include <QHeaderView>
#include <QMainWindow>
#include <QSortFilterProxyModel>
#include "../../definations/actiongroups.h"
#include "../../definations/toolbargroups.h"
#include "../../definations/discoitemdataroles.h"
#include "../../definations/resources.h"
#include "../../definations/menuicons.h"
#include "../../interfaces/iservicediscovery.h"
#include "../../interfaces/irosterchanger.h"
#include "../../interfaces/ivcard.h"
#include "../../utils/action.h"
#include "discoitemsmodel.h"
#include "ui_discoitemswindow.h"

class SortFilterProxyModel : 
  public QSortFilterProxyModel
{
public:
  SortFilterProxyModel(QObject *AParent):QSortFilterProxyModel(AParent) {};
  virtual bool hasChildren(const QModelIndex &AParent) const;
  virtual bool filterAcceptsRow(int ARow, const QModelIndex &AParent) const;
};

class DiscoItemsWindow :
  public QMainWindow,
  public IDiscoItemsWindow
{
  Q_OBJECT;
  Q_INTERFACES(IDiscoItemsWindow);
public:
  DiscoItemsWindow(IServiceDiscovery *ADiscovery, const Jid &AStreamJid, QWidget *AParent = NULL);
  ~DiscoItemsWindow();
  virtual QWidget *instance() { return this; }
  virtual Jid streamJid() const { return FStreamJid; }
  virtual ToolBarChanger *toolBarChanger() const { return FToolBarChanger; }
  virtual ToolBarChanger *actionsBarChanger() const { return FActionsBarChanger; }
  virtual void discover(const Jid AContactJid, const QString &ANode);
public:
  virtual QMenu *createPopupMenu() { return NULL; }
signals:
  virtual void discoverChanged(const Jid AContactJid, const QString &ANode);
  virtual void currentIndexChanged(const QModelIndex &AIndex);
  virtual void indexContextMenu(const QModelIndex &AIndex, Menu *AMenu);
  virtual void streamJidChanged(const Jid &ABefour, const Jid &AAftert);
  virtual void windowDestroyed(IDiscoItemsWindow *AWindow);
protected:
  void initialize();
  void createToolBarActions();
  void updateToolBarActions();
  void updateActionsBar();
protected slots:
  void onDiscoInfoReceived(const IDiscoInfo &ADiscoInfo);
  void onDiscoItemsReceived(const IDiscoItems &ADiscoItems);
  void onViewContextMenu(const QPoint &APos);
  void onCurrentIndexChanged(QModelIndex ACurrent, QModelIndex APrevious);
  void onToolBarActionTriggered(bool);
  void onComboReturnPressed();
  void onStreamJidChanged(const Jid &ABefour, const Jid &AAftert);
  void onSearchTimerTimeout();
private:
  QHeaderView *FHeader;
  Ui::DiscoItemsWindowClass ui;
private:
  IDataForms *FDataForms;
  IVCardPlugin *FVCardPlugin;
  IRosterChanger *FRosterChanger;
  IServiceDiscovery *FDiscovery;
private:
  Action *FMoveBack;
  Action *FMoveForward;
  Action *FDiscoverCurrent;
  Action *FReloadCurrent;
  Action *FDiscoInfo;
  Action *FAddContact;
  Action *FShowVCard;
  ToolBarChanger *FToolBarChanger;
  ToolBarChanger *FActionsBarChanger;
private:
  DiscoItemsModel *FModel;
  SortFilterProxyModel *FProxy;
private:
  Jid FStreamJid;
  int FCurrentStep;
  QTimer FSearchTimer;
  QList< QPair<Jid,QString> > FDiscoverySteps;
};

#endif // DISCOITEMSWINDOW_H
