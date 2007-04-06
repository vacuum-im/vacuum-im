#ifndef OPTIONSDIALOG_H
#define OPTIONSDIALOG_H

#include <QHash>
#include <QDialog>
#include <QLabel>
#include <QTreeWidget>
#include <QScrollArea>
#include <QStackedWidget>
#include <QDialogButtonBox>

class OptionsDialog : 
  public QDialog
{
  Q_OBJECT;

public:
  OptionsDialog(QWidget *AParent = NULL);
  ~OptionsDialog();

  void openNode(const QString &ANode, const QString &AName, 
    const QString &ADescription, const QIcon &AIcon, QWidget *AWidget);
  void closeNode(const QString &ANode);
  void showNode(const QString &ANode);
protected:
  QTreeWidgetItem *createTreeItem(const QString &ANode);
  QString nodeFullName(const QString &ANode);
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
  struct OptionsNode  
  {
    QString name;
    QString desc;
    QIcon icon;
    QWidget *widget;
  };
  QHash<QString, OptionsNode *> FNodes;
  QHash<QString, QTreeWidgetItem *> FNodesItems;
  QHash<QTreeWidgetItem *, int> FItemsStackIndex;
};

#endif // OPTIONSDIALOG_H
