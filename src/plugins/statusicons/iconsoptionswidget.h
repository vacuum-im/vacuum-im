#ifndef ICONSOPTIONSWIDGET_H
#define ICONSOPTIONSWIDGET_H

#include <QItemDelegate>
#include <definations/resources.h>
#include <definations/optionvalues.h>
#include <interfaces/istatusicons.h>
#include <interfaces/ioptionsmanager.h>
#include <utils/options.h>
#include <utils/iconstorage.h>
#include <utils/iconsetdelegate.h>
#include "ui_iconsoptionswidget.h"

class IconsetSelectableDelegate :
  public IconsetDelegate
{
public:
  IconsetSelectableDelegate(const QString &AStorage, const QStringList &ASubStorages, QObject *AParent = NULL);
  virtual QWidget *createEditor(QWidget *AParent, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const;
  virtual void setEditorData(QWidget *AEditor, const QModelIndex &AIndex) const;
  virtual void setModelData(QWidget *AEditor, QAbstractItemModel *AModel, const QModelIndex &AIndex) const;
  virtual void updateEditorGeometry(QWidget *AEditor, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const;
private:
  QString FStorage;
  QStringList FSubStorages;
};


class IconsOptionsWidget : 
  public QWidget,
  public IOptionsWidget
{
  Q_OBJECT;
  Q_INTERFACES(IOptionsWidget);
public:
  IconsOptionsWidget(IStatusIcons *AStatusIcons, QWidget *AParent);
  virtual QWidget* instance() { return this; }
public slots:
  virtual void apply();
  virtual void reset();
signals:
  void modified();
  void childApply();
  void childReset();
protected:
  void populateRulesTable(QTableWidget *ATable, IStatusIcons::RuleType ARuleType);
protected slots:
  void onAddUserRule();
  void onDeleteUserRule();
  void onDefaultListItemChanged(QListWidgetItem *AItem);
private:
  Ui::IconsOptionsWidgetClass ui;
private:
  IStatusIcons *FStatusIcons;
private:
  QList<QString> FSubStorages;
};

#endif // ICONSOPTIONSWIDGET_H
