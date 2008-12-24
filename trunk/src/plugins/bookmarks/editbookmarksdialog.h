#ifndef EDITBOOKMARKSDIALOG_H
#define EDITBOOKMARKSDIALOG_H

#include <QList>
#include <QDialog>
#include "../../interfaces/ibookmarks.h"
#include "../../utils/skin.h"
#include "ui_editbookmarksdialog.h"

class EditBookmarksDialog : 
  public QDialog
{
  Q_OBJECT;
public:
  EditBookmarksDialog(IBookMarks *ABookmarks, const Jid &AStreamJid, const QList<IBookMark> &AList, QWidget *AParent = NULL);
  ~EditBookmarksDialog();
  const Jid  &streamJid() const { return FStreamJid; }
signals:
  void dialogDestroyed();
protected:
  IBookMark getBookmarkFromRow(int ARow) const;
  void setBookmarkToRow(int ARow, const IBookMark &ABookmark);
protected slots:
  void onEditButtonClicked();
  void onDialogAccepted();
  void onBookmarksUpdated(const QString &AId, const Jid &AStreamJid, const QDomElement &AElement);
  void onBookmarksError(const QString &AId, const QString &AError);
  void onTableItemActivated(QTableWidgetItem *AItem);
private:
  Ui::EditBookmarksDialogClass ui;
private:
  IBookMarks *FBookmarks;
private:
  Jid FStreamJid;
  QString FRequestId;
};

#endif // EDITBOOKMARKSDIALOG_H
