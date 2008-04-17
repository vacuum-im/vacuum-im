#ifndef VIEWHISTORYWINDOW_H
#define VIEWHISTORYWINDOW_H

#include <QTimer>
#include <QSortFilterProxyModel>
#include "ui_viewhistorywindow.h"
#include "../../definations/archiveindextyperole.h"
#include "../../definations/actiongroups.h"
#include "../../interfaces/iroster.h"
#include "../../interfaces/imessenger.h"
#include "../../interfaces/isettings.h"
#include "../../interfaces/imessagearchiver.h"
#include "../../utils/skin.h"

class ViewHistoryWindow;

class SortFilterProxyModel :
  public QSortFilterProxyModel
{
public:
  SortFilterProxyModel(ViewHistoryWindow *AWindow, QObject *AParent = NULL);
protected:
  virtual bool filterAcceptsRow(int ARow, const QModelIndex &AParent) const;
private:
  ViewHistoryWindow *FWindow;
};

class ViewHistoryWindow : 
  public QMainWindow,
  public IArchiveWindow
{
  Q_OBJECT;
  Q_INTERFACES(IArchiveWindow);
public:
  ViewHistoryWindow(IMessageArchiver *AArchiver, const Jid &AStreamJid, QWidget *AParent = NULL);
  ~ViewHistoryWindow();
  virtual QMainWindow *instance() { return this; }
  virtual const Jid &streamJid() const { return FStreamJid; }
  virtual ToolBarChanger *groupsTools() const { return FGroupsTools; }
  virtual ToolBarChanger *messagesTools() const { return FMessagesTools->toolBarChanger(); }
  virtual bool isHeaderAccepted(const IArchiveHeader &AHeader) const;
  virtual QStandardItem *findHeaderItem(const IArchiveHeader &AHeader, QStandardItem *AParent = NULL) const;
  virtual int groupKind() const { return FGroupKind; }
  virtual void setGroupKind(int AGroupKind);
  virtual const IArchiveFilter &filter() const { return FFilter; }
  virtual void setFilter(const IArchiveFilter &AFilter);
  virtual void reload();
signals:
  virtual void groupKindChanged(int AGroupKind);
  virtual void filterChanged(const IArchiveFilter &AFilter);
  virtual void itemCreated(QStandardItem *AItem);
  virtual void itemContextMenu(QStandardItem *AItem, Menu *AMenu);
  virtual void itemChanged(QStandardItem *ACurrent, QStandardItem *ABefour);
  virtual void itemDestroyed(QStandardItem *AItem);
  virtual void windowDestroyed(IArchiveWindow *AWindow);
protected:
  void initialize();
  QList<IArchiveRequest> createRequests(const IArchiveFilter &AFilter) const;
  void divideRequests(const QList<IArchiveRequest> &ARequests, QList<IArchiveRequest> &ALocal, QList<IArchiveRequest> &AServer) const;
  bool loadServerHeaders(const IArchiveRequest &ARequest, const QString &AAfter = "");
  bool loadServerCollection(const IArchiveHeader &AHeader, const QString &AAfter = "");
  QStandardItem *findChildItem(int ARole, const QVariant &AValue, QStandardItem *AParent) const;
  QStandardItem *createSortItem(QStandardItem *AOwner);
  QStandardItem *createCustomItem(int AType, const QVariant &ADiaplay);
  QStandardItem *createDateGroup(const IArchiveHeader &AHeader, QStandardItem *AParent);
  QStandardItem *createContactGroup(const IArchiveHeader &AHeader, QStandardItem *AParent);
  QStandardItem *createHeaderParent(const IArchiveHeader &AHeader, QStandardItem *AParent);
  QStandardItem *createHeaderItem(const IArchiveHeader &AHeader);
  void updateHeaderItem(const IArchiveHeader &AHeader);
  void removeCustomItem(QStandardItem *AItem);
  void processRequests(const QList<IArchiveRequest> &ARequests);
  void processHeaders(const QList<IArchiveHeader> &AHeaders);
  void processCollection(const IArchiveCollection &ACollection, bool AAppend = false);
  void insertFilterWith(const IArchiveHeader &AHeader);
  void updateFilterWidgets();
  void clearModel();
  void rebuildModel();
  void createGroupKindMenu();
  void createHeaderActions();
  void updateHeaderActions();
protected slots:
  void onLocalCollectionSaved(const Jid &AStreamJid, const IArchiveHeader &AHeader);
  void onLocalCollectionRemoved(const Jid &AStreamJid, const IArchiveHeader &AHeader);
  void onServerHeadersLoaded(const QString &AId, const QList<IArchiveHeader> &AHeaders, const QString &ALast, int ACount);
  void onServerCollectionLoaded(const QString &AId, const IArchiveCollection &ACollection, const QString &ALast, int ACount);
  void onServerCollectionSaved(const QString &AId, const IArchiveHeader &AHeader);
  void onServerCollectionsRemoved(const QString &AId, const IArchiveRequest &ARequest);
  void onRequestFailed(const QString &AId, const QString &AError);
  void onCurrentItemChanged(const QModelIndex &ACurrent, const QModelIndex &ABefour);
  void onItemContextMenuRequested(const QPoint &APos);
  void onApplyFilterClicked();
  void onInvalidateTimeout();
  void onChangeGroupKindByAction(bool);
  void onHeaderActionTriggered(bool);
private:
  Ui::ViewHistoryWindowClass ui;
private:
  IRoster *FRoster;
  IViewWidget *FViewWidget;
  IToolBarWidget *FMessagesTools;
  IMessenger *FMessenger;
  ISettings *FSettings;
  IMessageArchiver *FArchiver;
private:
  QStandardItemModel *FModel;
  SortFilterProxyModel *FProxyModel;
private:
  Action *FFilterBy;
  Action *FRename;
  Action *FRemove;
  Action *FReload;
  Menu *FGroupKindMenu;
  ToolBarChanger *FGroupsTools;
private:
  int FGroupKind;
  QTimer FInvalidateTimer;
  Jid FStreamJid;
  IArchiveFilter FFilter;
  QHash<QString,IArchiveRequest> FHeaderRequests;
  QHash<QString,IArchiveHeader>FCollectionRequests;
  QHash<QString,IArchiveHeader> FRenameRequests;
  QHash<QString,IArchiveHeader> FRemoveRequests;
  QList<IArchiveRequest> FRequestList;
  QMap<IArchiveHeader,IArchiveCollection> FCollections;
  IArchiveHeader FCurrentHeader;
};

#endif // VIEWHISTORYWINDOW_H
