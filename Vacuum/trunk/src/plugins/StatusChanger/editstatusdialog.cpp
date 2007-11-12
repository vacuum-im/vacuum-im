#include "editstatusdialog.h"

#include <QHeaderView>
#include <QTableWidgetItem>
#include <QComboBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QInputDialog>
#include <QMessageBox>

#define TIR_STATUSID            Qt::UserRole
#define TIR_DELEGATE            Qt::UserRole + 1
#define TIR_VALUE               Qt::UserRole + 2

Delegate::Delegate(IStatusChanger *AStatusChanger, QObject *AParent /*= NULL*/)
  : QItemDelegate(AParent)
{
  FStatusChanger = AStatusChanger;
}

QWidget *Delegate::createEditor(QWidget *AParent, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const
{
  DelegateType type = (DelegateType)AIndex.data(TIR_DELEGATE).toInt();
  switch(type)
  {
  case DelegateIcon: 
    {
      return NULL;
    }
  case DelegateShow: 
    {
      QComboBox *comboBox = new QComboBox(AParent);
      comboBox->addItem(FStatusChanger->iconByShow(IPresence::Online),FStatusChanger->nameByShow(IPresence::Online),IPresence::Online);
      comboBox->addItem(FStatusChanger->iconByShow(IPresence::Chat),FStatusChanger->nameByShow(IPresence::Chat),IPresence::Chat);
      comboBox->addItem(FStatusChanger->iconByShow(IPresence::Away),FStatusChanger->nameByShow(IPresence::Away),IPresence::Away);
      comboBox->addItem(FStatusChanger->iconByShow(IPresence::DoNotDistrib),FStatusChanger->nameByShow(IPresence::DoNotDistrib),IPresence::DoNotDistrib);
      comboBox->addItem(FStatusChanger->iconByShow(IPresence::ExtendedAway),FStatusChanger->nameByShow(IPresence::ExtendedAway),IPresence::ExtendedAway);
      comboBox->addItem(FStatusChanger->iconByShow(IPresence::Invisible),FStatusChanger->nameByShow(IPresence::Invisible),IPresence::Invisible);
      comboBox->addItem(FStatusChanger->iconByShow(IPresence::Offline),FStatusChanger->nameByShow(IPresence::Offline),IPresence::Offline);
      comboBox->setEditable(false);
      return comboBox;
    }
  case DelegatePriority:
    {
      QSpinBox *spinBox = new QSpinBox(AParent);
      spinBox->setMinimum(-128);
      spinBox->setMaximum(128);
      return spinBox;
    }
  default: 
      return QItemDelegate::createEditor(AParent,AOption,AIndex);
  }
  
}

void Delegate::setEditorData(QWidget *AEditor, const QModelIndex &AIndex) const
{
  DelegateType type = (DelegateType)AIndex.data(TIR_DELEGATE).toInt();
  switch(type)
  {
  case DelegateShow: 
    {
      QComboBox *comboBox = qobject_cast<QComboBox *>(AEditor);
      if (comboBox)
      {
        int show = AIndex.data(TIR_VALUE).toInt();
        comboBox->setCurrentIndex(comboBox->findData(show));
      }
      break;
    }
  case DelegatePriority:
    {
      QSpinBox *spinBox = qobject_cast<QSpinBox *>(AEditor);
      if (spinBox)
        spinBox->setValue(AIndex.data(TIR_VALUE).toInt());
    }
  default: 
    QItemDelegate::setEditorData(AEditor,AIndex);
  }
}

void Delegate::setModelData(QWidget *AEditor, QAbstractItemModel *AModel, const QModelIndex &AIndex) const
{
  DelegateType type = (DelegateType)AIndex.data(TIR_DELEGATE).toInt();
  switch(type)
  {
  case DelegateName:
  case DelegateMessage:
    {
      QLineEdit *lineEdit = qobject_cast<QLineEdit *>(AEditor);
      if (lineEdit && !lineEdit->text().trimmed().isEmpty())
      {
        AModel->setData(AIndex, lineEdit->text(), Qt::DisplayRole);
        AModel->setData(AIndex, lineEdit->text(), TIR_VALUE);
      }
      break;
    }
  case DelegateShow: 
    {
      QComboBox *comboBox = qobject_cast<QComboBox *>(AEditor);
      if (comboBox)
      {
        int show = comboBox->itemData(comboBox->currentIndex()).toInt();
        AModel->setData(AIndex, FStatusChanger->iconByShow(show), Qt::DecorationRole);
        AModel->setData(AIndex, FStatusChanger->nameByShow(show), Qt::DisplayRole);
        AModel->setData(AIndex, show, TIR_VALUE);
      }
      break;
    }
  case DelegatePriority:
    {
      QSpinBox *spinBox = qobject_cast<QSpinBox *>(AEditor);
      if (spinBox)
      {
        AModel->setData(AIndex, spinBox->value(), Qt::DisplayRole);
        AModel->setData(AIndex, spinBox->value(), TIR_VALUE);
      }
    }
  default: 
    QItemDelegate::setModelData(AEditor,AModel,AIndex);
  }
}

void Delegate::updateEditorGeometry(QWidget *AEditor, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const
{
  DelegateType type = (DelegateType)AIndex.data(TIR_DELEGATE).toInt();
  switch(type)
  {
  case DelegateShow: 
    {
      AEditor->setGeometry(AOption.rect);
      break;
    }
  default:
    QItemDelegate::updateEditorGeometry(AEditor,AOption,AIndex);
  }
}

//EditStatusDialog
EditStatusDialog::EditStatusDialog(IStatusChanger *AStatusChanger)
{
  setAttribute(Qt::WA_DeleteOnClose,true);
  setupUi(this);
  FStatusChanger = AStatusChanger;
  
  tblStatus->setColumnCount(5);
  tblStatus->setHorizontalHeaderLabels(QStringList() <<tr("Icon")<<tr("Name")<<tr("Show")<<tr("Message")<<tr("Priority"));
  tblStatus->setSelectionBehavior(QAbstractItemView::SelectRows);
  tblStatus->setSelectionMode(QAbstractItemView::SingleSelection);
  tblStatus->verticalHeader()->hide();
  tblStatus->setWordWrap(true);
  tblStatus->setItemDelegate(new Delegate(AStatusChanger,tblStatus));

  int row = 0;
  QList<int> statuses = FStatusChanger->statusItems();
  QMultiMap<QString,int> statusOrdered;
  foreach (int statusId, statuses)
  {
    if (statusId > NULL_STATUS_ID)
    {
      RowStatus *status = new RowStatus;
      status->id = statusId;
      status->name = FStatusChanger->statusItemName(statusId);
      status->show = FStatusChanger->statusItemShow(statusId);
      status->priority = FStatusChanger->statusItemPriority(statusId);
      status->text = FStatusChanger->statusItemText(statusId);
      status->icon = FStatusChanger->statusItemIcon(statusId);
      if (status->icon.isNull())
        status->icon = FStatusChanger->iconByShow(status->show);
      FStatusItems.insert(statusId,status);

      QString sortString = QString("%1-%2-%3").arg(statusId > MAX_STANDART_STATUS_ID ? 0 : 1)
                                              .arg(status->show!=IPresence::Offline ? status->show : 10,5,10,QChar('0'))
                                              .arg(128-status->priority,3,10,QChar('0'));
      statusOrdered.insert(sortString,statusId);
    }
  }
  foreach (int statusId, statusOrdered)
  {
    tblStatus->insertRow(tblStatus->rowCount());

    RowStatus *status = FStatusItems.value(statusId);

    QTableWidgetItem *icon = new QTableWidgetItem;
    icon->setData(Qt::DecorationRole, status->icon);
    icon->setData(TIR_STATUSID,statusId);
    icon->setData(TIR_DELEGATE,Delegate::DelegateIcon);
    icon->setData(TIR_VALUE,status->icon);
    tblStatus->setItem(row,0,icon);

    QTableWidgetItem *name = new QTableWidgetItem;
    name->setData(Qt::DisplayRole, status->name);
    name->setData(TIR_DELEGATE,Delegate::DelegateName);
    name->setData(TIR_VALUE,status->name);
    tblStatus->setItem(row,1,name);

    QTableWidgetItem *show = new QTableWidgetItem;
    show->setData(Qt::DisplayRole, FStatusChanger->nameByShow(status->show));
    show->setData(Qt::DecorationRole, FStatusChanger->iconByShow(status->show));
    show->setData(TIR_DELEGATE,Delegate::DelegateShow);
    show->setData(TIR_VALUE,status->show);
    tblStatus->setItem(row,2,show);

    QTableWidgetItem *message = new QTableWidgetItem;
    message->setData(Qt::DisplayRole, status->text);
    message->setData(TIR_DELEGATE,Delegate::DelegateMessage);
    message->setData(TIR_VALUE,status->text);
    tblStatus->setItem(row,3,message);

    QTableWidgetItem *priority = new QTableWidgetItem;
    priority->setTextAlignment(Qt::AlignCenter);
    priority->setData(Qt::DisplayRole, status->priority);
    priority->setData(TIR_DELEGATE,Delegate::DelegatePriority);
    priority->setData(TIR_VALUE,status->priority);
    tblStatus->setItem(row,4,priority);

    if (statusId > MAX_STANDART_STATUS_ID)
    {
      icon->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
      name->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
      show->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
      message->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
      priority->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
    }
    else
    {
      icon->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
      name->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
      show->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
      message->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
      priority->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
    }

    row++;
  }
  tblStatus->horizontalHeader()->setResizeMode(0,QHeaderView::ResizeToContents);
  tblStatus->horizontalHeader()->setResizeMode(1,QHeaderView::ResizeToContents);
  tblStatus->horizontalHeader()->setResizeMode(2,QHeaderView::ResizeToContents);
  tblStatus->horizontalHeader()->setResizeMode(3,QHeaderView::Stretch);
  tblStatus->horizontalHeader()->setResizeMode(4,QHeaderView::ResizeToContents);
  
  connect(pbtAdd,SIGNAL(clicked(bool)),SLOT(onAddbutton(bool)));
  connect(pbtDelete,SIGNAL(clicked(bool)),SLOT(onDeleteButton(bool)));
  connect(dbtDialogButtons,SIGNAL(accepted()),SLOT(onDialogButtonsBoxAccepted()));
}

EditStatusDialog::~EditStatusDialog()
{
  qDeleteAll(FStatusItems);
}

void EditStatusDialog::onAddbutton(bool)
{
  QString statusName = QInputDialog::getText(this,tr("Enter status name"),tr("Status name:"));
  if (!statusName.isEmpty())
  {
    if (FStatusChanger->statusByName(statusName) == NULL_STATUS_ID)
    {
      int row = tblStatus->rowCount();
      tblStatus->insertRow(row);

      QTableWidgetItem *icon = new QTableWidgetItem;
      icon->setData(Qt::DecorationRole, QIcon());
      icon->setData(TIR_STATUSID,NULL_STATUS_ID);
      icon->setData(TIR_DELEGATE,Delegate::DelegateIcon);
      icon->setData(TIR_VALUE,QIcon());
      tblStatus->setItem(row,0,icon);

      QTableWidgetItem *name = new QTableWidgetItem;
      name->setData(Qt::DisplayRole, statusName);
      name->setData(TIR_DELEGATE,Delegate::DelegateName);
      name->setData(TIR_VALUE,statusName);
      tblStatus->setItem(row,1,name);

      QTableWidgetItem *show = new QTableWidgetItem;
      show->setData(Qt::DisplayRole, FStatusChanger->nameByShow(IPresence::Online));
      show->setData(Qt::DecorationRole, FStatusChanger->iconByShow(IPresence::Online));
      show->setData(TIR_DELEGATE,Delegate::DelegateShow);
      show->setData(TIR_VALUE,IPresence::Online);
      tblStatus->setItem(row,2,show);

      QTableWidgetItem *message = new QTableWidgetItem;
      message->setData(Qt::DisplayRole, statusName);
      message->setData(TIR_DELEGATE,Delegate::DelegateMessage);
      message->setData(TIR_VALUE,statusName);
      tblStatus->setItem(row,3,message);

      QTableWidgetItem *priority = new QTableWidgetItem;
      priority->setTextAlignment(Qt::AlignCenter);
      priority->setData(Qt::DisplayRole, 100);
      priority->setData(TIR_DELEGATE,Delegate::DelegatePriority);
      priority->setData(TIR_VALUE,100);
      tblStatus->setItem(row,4,priority);

      tblStatus->editItem(message);
    }
    else
      QMessageBox::warning(this,tr("Wrong status name"),tr("Status with name '<b>%1</b>' allready exists").arg(Qt::escape(statusName)));

  }
}

void EditStatusDialog::onDeleteButton(bool)
{
  QList<QTableWidgetItem *> tableItems = tblStatus->selectedItems();
  foreach (QTableWidgetItem *tableItem, tableItems)
  {
    if (tableItem->data(TIR_STATUSID).isValid())
    {
      int statusId = tableItem->data(TIR_STATUSID).toInt();
      if (statusId == NULL_STATUS_ID)
      {
        tblStatus->removeRow(tableItem->row());
      }
      else if (statusId <= MAX_STANDART_STATUS_ID)
      {
        QMessageBox::information(this,tr("Can't delete status"),tr("You can not delete standard statuses."));
      }
      else if (FStatusChanger->activeStatusItems().contains(statusId))
      {
        QMessageBox::information(this,tr("Can't delete status"),tr("You can not delete active statuses."));
      }
      else if (FStatusItems.contains(statusId))
      {
        int button = QMessageBox::question(this,tr("Delete status"),
          tr("You are assured that wish to remove a status '<b>%1</b>'?").arg(Qt::escape(FStatusItems.value(statusId)->name)),
          QMessageBox::Yes | QMessageBox::No);
        if (button == QMessageBox::Yes)
        {
          FDeletedStatuses.append(statusId);
          tblStatus->removeRow(tableItem->row());
        }
      }
      break;
    }
  }
}

void EditStatusDialog::onDialogButtonsBoxAccepted()
{
  foreach (int statusId, FDeletedStatuses)
    FStatusChanger->removeStatusItem(statusId);

  for (int i=0; i<tblStatus->rowCount(); i++)
  {
    int statusId = tblStatus->item(i,0)->data(TIR_STATUSID).toInt();
    
    QString name = tblStatus->item(i,1)->data(TIR_VALUE).toString();
    int show = tblStatus->item(i,2)->data(TIR_VALUE).toInt();
    QString text = tblStatus->item(i,3)->data(TIR_VALUE).toString();
    int priority = tblStatus->item(i,4)->data(TIR_VALUE).toInt();
    
    RowStatus *status = FStatusItems.value(statusId,NULL);
    if (!status)
    {
      if (!name.isEmpty() && !text.isEmpty())
        FStatusChanger->addStatusItem(name,show,text,priority);
    }
    else if (status->name != name || status->show != show || status->text != text || status->priority != priority)
    {
      FStatusChanger->updateStatusItem(statusId,name,show,text,priority);
    }
  }
  accept();
}

