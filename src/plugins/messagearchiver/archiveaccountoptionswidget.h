#ifndef ARCHIVEOPTIONS_H
#define ARCHIVEOPTIONS_H

#include <QItemDelegate>
#include <interfaces/imessagearchiver.h>
#include <interfaces/ioptionsmanager.h>
#include "ui_archiveaccountoptionswidget.h"

class ArchiveDelegate :
	public QItemDelegate
{
	Q_OBJECT;
public:
	static QString saveModeName(const QString &ASaveMode);
	static void updateComboBox(int AColumn, QComboBox *AComboBox);
public:
	ArchiveDelegate(IMessageArchiver *AArchiver, QObject *AParent = NULL);
	QWidget *createEditor(QWidget *AParent, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const;
	void setEditorData(QWidget *AEditor, const QModelIndex &AIndex) const;
	void setModelData(QWidget *AEditor, QAbstractItemModel *AModel, const QModelIndex &AIndex) const;
	void updateEditorGeometry(QWidget *AEditor, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const;
private:
	IMessageArchiver *FArchiver;
};

class ArchiveAccountOptionsWidget :
	public QWidget,
	public IOptionsDialogWidget
{
	Q_OBJECT;
	Q_INTERFACES(IOptionsDialogWidget);
public:
	ArchiveAccountOptionsWidget(IMessageArchiver *AArchiver, const Jid &AStreamJid, QWidget *AParent);
	~ArchiveAccountOptionsWidget();
	virtual QWidget* instance() { return this; }
public slots:
	virtual void apply();
	virtual void reset();
signals:
	void modified();
	void childApply();
	void childReset();
protected:
	void updateWidget();
	void updateColumnsSize();
	void updateItemPrefs(const Jid &AItemJid, const QString &ASaveMode);
	void removeItemPrefs(const Jid &AItemJid);
protected slots:
	void onAddItemPrefClicked();
	void onRemoveItemPrefClicked();
	void onArchivePrefsOpened(const Jid &AStreamJid);
	void onArchivePrefsChanged(const Jid &AStreamJid);
	void onArchivePrefsClosed(const Jid &AStreamJid);
	void onArchiveRequestCompleted(const QString &AId);
	void onArchiveRequestFailed(const QString &AId, const XmppError &AError);
private:
	Ui::ArchiveAccountOptionsWidgetClass ui;
private:
	IMessageArchiver *FArchiver;
private:
	Jid FStreamJid;
	XmppError FLastError;
	QList<QString> FSaveRequests;
	QHash<Jid,QTableWidgetItem *> FTableItems;
};

#endif // ARCHIVEOPTIONS_H
