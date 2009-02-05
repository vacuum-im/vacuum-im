#ifndef OPTIONSDIALOG_H
#define OPTIONSDIALOG_H

#include <QHash>
#include <QDialog>
#include <QLabel>
#include <QTreeWidget>
#include <QScrollArea>
#include <QStackedWidget>
#include <QDialogButtonBox>
#include "../../definations/resources.h"
#include "../../definations/menuicons.h"
#include "../../utils/iconstorage.h"

class OptionsDialog : 
  public QDialog
{
  Q_OBJECT;
public:
  OptionsDialog(QWidget *AParent = NULL);
  ~OptionsDialog();
  void openNode(const QString &ANode, const QString &AName, const QString &ADescription, const QString &AIcon, QWidget *AWidget);
  void closeNode(const QString &ANode);
  void showNode(const QString &ANode);
signals:
  void closed();
protected:
  QTreeWidgetItem *createTreeItem(const QString &ANode);
  QString nodeFullName(const QString &ANode);
  bool canExpandVertically(const QWidget *AWidget) const;
  void updateOptionsSize(QWidget *AWidget);
protected:
  virtual void resizeEvent(QResizeEvent *AEvent);
protected slots:
  void onDialogButtonClicked(QAbstractButton *AButton);
  void onCurrentItemChanged(QTreeWidgetItem *ACurrent, QTreeWidgetItem *APrevious);
private:
  QLabel *lblInfo;
  QScrollArea *scaScroll;
  QStackedWidget *stwOptions;  
  QTreeWidget *trwNodes;
  QDialogButtonBox *dbbButtons;
private:
  struct OptionsNode {
    QString icon;
    QString name;
    QString desc;
    QWidget *widget;
  };
  QHash<QString, OptionsNode *> FNodes;
  QHash<QString, QTreeWidgetItem *> FNodeItems;
  QHash<QTreeWidgetItem *, int> FItemsStackIndex;
};

#endif // OPTIONSDIALOG_H
