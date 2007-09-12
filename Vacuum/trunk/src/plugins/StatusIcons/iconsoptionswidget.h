#ifndef ICONSOPTIONSWIDGET_H
#define ICONSOPTIONSWIDGET_H

#include <QStringList>
#include <QItemDelegate>
#include <QSignalMapper>
#include "../../interfaces/istatusicons.h"
#include "../../utils/iconsetdelegate.h"
#include "../../utils/skin.h"
#include "ui_iconsoptionswidget.h"

class IconsetSelectableDelegate :
  public IconsetDelegate
{
public:
  IconsetSelectableDelegate(const QStringList &AIconFiles, QObject *AParent = NULL);
  virtual QWidget *createEditor(QWidget *AParent, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const;
  virtual void setEditorData(QWidget *AEditor, const QModelIndex &AIndex) const;
  virtual void setModelData(QWidget *AEditor, QAbstractItemModel *AModel, const QModelIndex &AIndex) const;
  virtual void updateEditorGeometry(QWidget *AEditor, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const;
private:
  QStringList FIconFiles;
};


class IconsOptionsWidget : 
  public QWidget
{
  Q_OBJECT;

public:
  IconsOptionsWidget(IStatusIcons *AStatusIcons);
  void apply();
protected:
  void populateRulesTable(QTableWidget *ATable, const QStringList &AIconFiles, IStatusIcons::RuleType ARuleType);
  void applyTableRules(QTableWidget *ATable,IStatusIcons::RuleType ARuleType);
protected slots:
  void onAddRule(QWidget *ATable);
  void onDeleteRule(QWidget *ATable);
private:
  Ui::IconsOptionsWidgetClass ui;
private:
  IStatusIcons *FStatusIcons;
private:
  QStringList FIconFiles;
  QSignalMapper *FAddRuleMapper;
  QSignalMapper *FDeleteRuleMapper;
};

#endif // ICONSOPTIONSWIDGET_H
