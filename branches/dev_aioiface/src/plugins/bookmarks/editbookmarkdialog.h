#ifndef EDITBOOKMARKDIALOG_H
#define EDITBOOKMARKDIALOG_H

#include <QDialog>
#include <interfaces/ibookmarks.h>
#include "ui_editbookmarkdialog.h"

class EditBookmarkDialog :
			public QDialog
{
	Q_OBJECT;
public:
	EditBookmarkDialog(IBookmark *ABookmark, QWidget *AParent = NULL);
	~EditBookmarkDialog();
protected slots:
	void onGroupBoxClicked(bool AChecked);
	void onDialogAccepted();
private:
	Ui::EditBookmarkDialogClass ui;
private:
	IBookmark *FBookmark;
};

#endif // EDITBOOKMARKDIALOG_H
