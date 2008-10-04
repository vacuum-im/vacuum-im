#ifndef DISCOITEMSWINDOW_H
#define DISCOITEMSWINDOW_H

#include <QHeaderView>
#include <QMainWindow>
#include "../../definations/discotreeitemsdataroles.h"
#include "../../interfaces/iservicediscovery.h"
#include "../../interfaces/irosterchanger.h"
#include "../../interfaces/ivcard.h"
#include "../../utils/action.h"
#include "ui_discoitemswindow.h"

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
  virtual QTreeWidgetItem *currentTreeItem() const { return ui.trwItems->currentItem(); }
  virtual bool useCache() const { return FUseCache->isChecked(); }
  virtual void setUseCache(bool AUse);
  virtual void discover(const Jid AContactJid, const QString &ANode);
signals:
  virtual void discoverChanged(const Jid AContactJid, const QString &ANode);
  virtual void treeItemCreated(QTreeWidgetItem *ATreeItem);
  virtual void treeItemChanged(QTreeWidgetItem *ATreeItem);
  virtual void treeItemSelected(QTreeWidgetItem *ACurrent, QTreeWidgetItem *APrevious);
  virtual void treeItemContextMenu(QTreeWidgetItem *ATreeItem, Menu *AMenu);
  virtual void treeItemDestroyed(QTreeWidgetItem *ATreeItem);
  virtual void streamJidChanged(const Jid &ABefour, const Jid &AAftert);
public:
  virtual QMenu *createPopupMenu() { return NULL; }
protected:
  void initialize();
  bool isNeedRequestInfo(const Jid AContactJid, const QString &ANode) const;
  bool isNeedRequestItems(const Jid AContactJid, const QString &ANode) const;
  void requestDiscoInfo(const Jid AContactJid, const QString &ANode);
  void requestDiscoItems(const Jid AContactJid, const QString &ANode);
  QTreeWidgetItem *createTreeItem(const IDiscoItem &ADiscoItem);
  void updateDiscoInfo(const IDiscoInfo &ADiscoInfo);
  void updateDiscoItems(const IDiscoItems &ADiscoItems);
  void destroyTreeItem(QTreeWidgetItem *ATreeItem);
  void createToolBarActions();
  void updateToolBarActions();
  void updateActionsBar();
protected slots:
  void onDiscoInfoReceived(const IDiscoInfo &ADiscoInfo);
  void onDiscoItemsReceived(const IDiscoItems &ADiscoItems);
  void onTreeItemContextMenu(const QPoint &APos);
  void onTreeItemExpanded(QTreeWidgetItem *AItem);
  void onCurrentTreeItemChanged(QTreeWidgetItem *ACurrent, QTreeWidgetItem *APrevious);
  void onToolBarActionTriggered(bool);
  void onComboReturnPressed();
  void onStreamJidChanged(const Jid &ABefour, const Jid &AAftert);
  void onHeaderSectionClicked(int AColumn);
private:
  QHeaderView *FHeader;
  Ui::DiscoItemsWindowClass ui;
private:
  IVCardPlugin *FVCardPlugin;
  IRosterChanger *FRosterChanger;
  IServiceDiscovery *FDiscovery;
private:
  Action *FMoveBack;
  Action *FMoveForward;
  Action *FDiscoverCurrent;
  Action *FReloadCurrent;
  Action *FUseCache;
  Action *FDiscoInfo;
  Action *FAddContact;
  Action *FShowVCard;
  ToolBarChanger *FToolBarChanger;
  ToolBarChanger *FActionsBarChanger;
private:
  Jid FStreamJid;
  int FCurrentStep;
  QList< QPair<Jid,QString> > FDiscoverySteps;
  QHash<Jid, QHash<QString,QTreeWidgetItem *> > FTreeItems;
};

#endif // DISCOITEMSWINDOW_H
