#ifndef OPTIONSDIALOG_H
#define OPTIONSDIALOG_H

#include <QHash>
#include <QLabel>
#include <QDialog>
#include <QTreeView>
#include <QScrollArea>
#include <QStackedWidget>
#include <QDialogButtonBox>
#include <QStandardItemModel>
#include <QSortFilterProxyModel>
#include <definations/resources.h>
#include <definations/menuicons.h>
#include <utils/iconstorage.h>

struct OptionsNode {
  int order;
  QString icon;
  QString name;
  QString desc;
};

class SettingsPlugin;

class SortFilterProxyModel : 
  public QSortFilterProxyModel
{
  Q_OBJECT;
public:
  SortFilterProxyModel(QObject *AParent) : QSortFilterProxyModel(AParent) {};
protected:
  virtual bool lessThan(const QModelIndex &ALeft, const QModelIndex &ARight) const;
};

class OptionsDialog : 
  public QDialog
{
  Q_OBJECT;
public:
  OptionsDialog(SettingsPlugin *ASessingsPlugin, QWidget *AParent = NULL);
  ~OptionsDialog();
  void openNode(const QString &ANode, const QString &AName, const QString &ADescription, const QString &AIcon, int AOrder);
  void closeNode(const QString &ANode);
  void showNode(const QString &ANode);
signals:
  void closed();
protected:
  QStandardItem *createNodeItem(const QString &ANode);
  QString nodeFullName(const QString &ANode);
  bool canExpandVertically(const QWidget *AWidget) const;
protected:
  virtual void showEvent(QShowEvent *AEvent);
protected slots:
  void onDialogButtonClicked(QAbstractButton *AButton);
  void onCurrentItemChanged(const QModelIndex &ACurrent, const QModelIndex &APrevious);
private:
  SettingsPlugin *FSettingsPlugin;
private:
  QLabel *lblInfo;
  QTreeView *trvNodes;
  QScrollArea *scaScroll;
  QDialogButtonBox *dbbButtons;
  QStandardItemModel *simNodes;
  SortFilterProxyModel *FProxyModel;
private:
  QMap<QString, OptionsNode *> FNodes;
  QMap<QString, QStandardItem *> FNodeItems;
  QMap<QStandardItem *, QWidget *> FItemWidget;
};

#endif // OPTIONSDIALOG_H
