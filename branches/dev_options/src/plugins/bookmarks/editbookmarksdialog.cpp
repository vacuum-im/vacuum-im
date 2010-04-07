#include "editbookmarksdialog.h"

#include <QHeaderView>
#include <QMessageBox>

#define C_NAME                  0
#define C_VALUE                 1
#define C_NICK                  2

#define TDR_NAME                Qt::UserRole+1
#define TDR_AUTO                Qt::UserRole+2
#define TDR_NICK                Qt::UserRole+3
#define TDR_PASSWORD            Qt::UserRole+4
#define TDR_CONF                Qt::UserRole+5
#define TDR_URL                 Qt::UserRole+6

EditBookmarksDialog::EditBookmarksDialog(IBookMarks *ABookmarks, const Jid &AStreamJid, 
                                         const QList<IBookMark> &AList, QWidget *AParent) : QDialog(AParent)
{
  ui.setupUi(this);
  setAttribute(Qt::WA_DeleteOnClose,true);
  setWindowTitle(tr("Edit bookmarks - %1").arg(AStreamJid.bare()));
  IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(this,MNI_BOOKMARKS_EDIT,0,0,"windowIcon");

  FBookmarks = ABookmarks;
  FStreamJid = AStreamJid;

  ui.tbwBookmarks->setRowCount(AList.count());
  for (int row=0; row<AList.count(); row++)
  {
    IBookMark bookmark = AList.at(row);
    setBookmarkToRow(row,bookmark);
  } 
  ui.tbwBookmarks->horizontalHeader()->setResizeMode(C_NAME,QHeaderView::ResizeToContents);
  ui.tbwBookmarks->horizontalHeader()->setResizeMode(C_VALUE,QHeaderView::Stretch);
  ui.tbwBookmarks->horizontalHeader()->setResizeMode(C_NICK,QHeaderView::ResizeToContents);

  connect(FBookmarks->instance(),SIGNAL(bookmarksUpdated(const QString &, const Jid &, const QDomElement &)),
    SLOT(onBookmarksUpdated(const QString &, const Jid &, const QDomElement &)));
  connect(FBookmarks->instance(),SIGNAL(bookmarksError(const QString &, const QString &)),
    SLOT(onBookmarksError(const QString &, const QString &)));

  connect(ui.pbtAdd,SIGNAL(clicked()),SLOT(onEditButtonClicked()));
  connect(ui.pbtEdit,SIGNAL(clicked()),SLOT(onEditButtonClicked()));
  connect(ui.pbtDelete,SIGNAL(clicked()),SLOT(onEditButtonClicked()));
  connect(ui.pbtMoveUp,SIGNAL(clicked()),SLOT(onEditButtonClicked()));
  connect(ui.pbtMoveDown,SIGNAL(clicked()),SLOT(onEditButtonClicked()));
  connect(ui.bbxButtons,SIGNAL(accepted()),SLOT(onDialogAccepted()));

  connect(ui.tbwBookmarks,SIGNAL(itemActivated(QTableWidgetItem *)),SLOT(onTableItemActivated(QTableWidgetItem *)));
}

EditBookmarksDialog::~EditBookmarksDialog()
{
  emit dialogDestroyed();
}

IBookMark EditBookmarksDialog::getBookmarkFromRow(int ARow) const
{
  IBookMark bookmark;
  QTableWidgetItem *tableItem = ui.tbwBookmarks->item(ARow,C_NAME);
  bookmark.name = tableItem->data(TDR_NAME).toString();
  bookmark.autojoin = tableItem->data(TDR_AUTO).toBool();
  bookmark.nick = tableItem->data(TDR_NICK).toString();
  bookmark.password = tableItem->data(TDR_PASSWORD).toString();
  bookmark.conference = tableItem->data(TDR_CONF).toString();
  bookmark.url = tableItem->data(TDR_URL).toString();
  return bookmark;
}

void EditBookmarksDialog::setBookmarkToRow(int ARow, const IBookMark &ABookmark)
{
  QTableWidgetItem *nameItem = new QTableWidgetItem;
  nameItem->setText(ABookmark.name);
  if (ABookmark.conference.isEmpty())
    nameItem->setIcon(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_BOOKMARKS_URL));
  else
    nameItem->setIcon(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_BOOKMARKS_ROOM));
  if (!ABookmark.conference.isEmpty() && ABookmark.autojoin)
  {
    QFont font = nameItem->font();
    font.setBold(true);
    nameItem->setFont(font);
  }
  nameItem->setData(TDR_NAME,ABookmark.name);
  nameItem->setData(TDR_AUTO,ABookmark.autojoin);
  nameItem->setData(TDR_NICK,ABookmark.nick);
  nameItem->setData(TDR_PASSWORD,ABookmark.password);
  nameItem->setData(TDR_CONF,ABookmark.conference);
  nameItem->setData(TDR_URL,ABookmark.url);
  ui.tbwBookmarks->setItem(ARow,C_NAME,nameItem);
  QTableWidgetItem *valueItem = new QTableWidgetItem;
  valueItem->setText(ABookmark.conference.isEmpty() ? ABookmark.url : ABookmark.conference);
  ui.tbwBookmarks->setItem(nameItem->row(),C_VALUE,valueItem);
  QTableWidgetItem *nickItem = new QTableWidgetItem;
  nickItem->setText(ABookmark.nick);
  ui.tbwBookmarks->setItem(nameItem->row(),C_NICK,nickItem);
}

void EditBookmarksDialog::onEditButtonClicked()
{
  QPushButton *button = qobject_cast<QPushButton *>(sender());
  if (button == ui.pbtAdd)
  {
    IBookMark bookmark;
    if (FBookmarks->execEditBookmarkDialog(&bookmark,this) == QDialog::Accepted)
    {
      ui.tbwBookmarks->setRowCount(ui.tbwBookmarks->rowCount()+1);
      setBookmarkToRow(ui.tbwBookmarks->rowCount()-1,bookmark);
    }
  }
  else if (button == ui.pbtEdit)
  {
    int row = ui.tbwBookmarks->currentRow();
    if (row >=0)
    {
      IBookMark bookmark = getBookmarkFromRow(row);
      if (FBookmarks->execEditBookmarkDialog(&bookmark,this) == QDialog::Accepted)
        setBookmarkToRow(row,bookmark);
    }
  }
  else if (button == ui.pbtDelete)
  {
    QTableWidgetItem *tableItem = ui.tbwBookmarks->currentItem();
    if (tableItem)
      ui.tbwBookmarks->removeRow(tableItem->row());
  }
  else if (button == ui.pbtMoveUp)
  {
    QTableWidgetItem *tableItem = ui.tbwBookmarks->currentItem();
    if (tableItem && tableItem->row()>0)
    {
      int row = tableItem->row();
      IBookMark bookmark1 = getBookmarkFromRow(row);
      IBookMark bookmark2 = getBookmarkFromRow(row-1);
      setBookmarkToRow(row,bookmark2);
      setBookmarkToRow(row-1,bookmark1);
      ui.tbwBookmarks->setCurrentCell(row-1,C_NAME);
    }
  }
  else if (button == ui.pbtMoveDown)
  {
    QTableWidgetItem *tableItem = ui.tbwBookmarks->currentItem();
    if (tableItem && tableItem->row()<ui.tbwBookmarks->rowCount()-1)
    {
      int row = tableItem->row();
      IBookMark bookmark1 = getBookmarkFromRow(row);
      IBookMark bookmark2 = getBookmarkFromRow(row+1);
      setBookmarkToRow(row,bookmark2);
      setBookmarkToRow(row+1,bookmark1);
      ui.tbwBookmarks->setCurrentCell(row+1,C_NAME);
    }
  }
}

void EditBookmarksDialog::onDialogAccepted()
{
  QList<IBookMark> bookmarks;
  for (int row=0; row<ui.tbwBookmarks->rowCount(); row++)
    bookmarks.append(getBookmarkFromRow(row));

  FRequestId = FBookmarks->setBookmarks(FStreamJid,bookmarks);
  if (!FRequestId.isEmpty())
  {
    ui.pbtAdd->setEnabled(false);
    ui.pbtEdit->setEnabled(false);
    ui.pbtDelete->setEnabled(false);
    ui.pbtMoveUp->setEnabled(false);
    ui.pbtMoveDown->setEnabled(false);
    ui.tbwBookmarks->setEnabled(false);
    ui.bbxButtons->setStandardButtons(QDialogButtonBox::Cancel);
  }
  else
    QMessageBox::warning(this,tr("Bookmarks not saved"),tr("Cant save bookmarks to server"));
}

void EditBookmarksDialog::onBookmarksUpdated(const QString &AId, const Jid &/*AStreamJid*/, const QDomElement &/*AElement*/)
{
  if (AId == FRequestId)
    accept();
}

void EditBookmarksDialog::onBookmarksError(const QString &AId, const QString &AError)
{
  if (AId == FRequestId)
  {
    FRequestId.clear();
    ui.pbtAdd->setEnabled(true);
    ui.pbtEdit->setEnabled(true);
    ui.pbtDelete->setEnabled(true);
    ui.pbtMoveUp->setEnabled(true);
    ui.pbtMoveDown->setEnabled(true);
    ui.tbwBookmarks->setEnabled(true);
    ui.bbxButtons->setStandardButtons(QDialogButtonBox::Save|QDialogButtonBox::Cancel);
    QMessageBox::warning(this,tr("Bookmarks not saved"),tr("Cant save bookmarks to server. %1").arg(AError));
  }
}

void EditBookmarksDialog::onTableItemActivated(QTableWidgetItem *AItem)
{
  IBookMark bookmark = getBookmarkFromRow(AItem->row());
  if (FBookmarks->execEditBookmarkDialog(&bookmark,this) == QDialog::Accepted)
    setBookmarkToRow(AItem->row(),bookmark);
}
