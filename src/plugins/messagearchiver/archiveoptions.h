#ifndef ARCHIVEOPTIONS_H
#define ARCHIVEOPTIONS_H

#include <QItemDelegate>
#include <definations/namespaces.h>
#include <interfaces/imessagearchiver.h>
#include <interfaces/ioptionsmanager.h>
#include "ui_archiveoptions.h"

class ArchiveDelegate :
			public QItemDelegate
{
	Q_OBJECT;
public:
	ArchiveDelegate(IMessageArchiver *AArchiver, QObject *AParent = NULL);
	QWidget *createEditor(QWidget *AParent, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const;
	void setEditorData(QWidget *AEditor, const QModelIndex &AIndex) const;
	void setModelData(QWidget *AEditor, QAbstractItemModel *AModel, const QModelIndex &AIndex) const;
	void updateEditorGeometry(QWidget *AEditor, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const;
public:
	void updateComboBox(int AColumn, QComboBox *AComboBox) const;
protected slots:
	void onExpireIndexChanged(int AIndex);
private:
	IMessageArchiver *FArchiver;
};


class ArchiveOptions :
			public QWidget,
			public IOptionsWidget
{
	Q_OBJECT;
	Q_INTERFACES(IOptionsWidget);
public:
	ArchiveOptions(IMessageArchiver *AArchiver, const Jid &AStreamJid, QWidget *AParent);
	~ArchiveOptions();
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
protected slots:
	void onAddItemPrefClicked();
	void onRemoveItemPrefClicked();
	void onArchiveAutoSaveChanged(const Jid &AStreamJid, bool AAutoSave);
	void onArchivePrefsChanged(const Jid &AStreamJid, const IArchiveStreamPrefs &APrefs);
	void onArchiveItemPrefsChanged(const Jid &AStreamJid, const Jid &AItemJid, const IArchiveItemPrefs &APrefs);
	void onArchiveItemPrefsRemoved(const Jid &AStreamJid, const Jid &AItemJid);
	void onArchiveRequestCompleted(const QString &AId);
	void onArchiveRequestFailed(const QString &AId, const QString &AError);
private:
	Ui::ArchiveOptionsClass ui;
private:
	IMessageArchiver *FArchiver;
private:
	Jid FStreamJid;
	QString FLastError;
	QList<QString> FSaveRequests;
	QHash<Jid,QTableWidgetItem *> FTableItems;
};

#endif // ARCHIVEOPTIONS_H
