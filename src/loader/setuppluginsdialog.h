#ifndef SETUPPLUGINSDIALOG_H
#define SETUPPLUGINSDIALOG_H

#include <QDialog>
#include <QDomDocument>
#include <QStandardItemModel>
#include <QSortFilterProxyModel>
#include <interfaces/ipluginmanager.h>
#include "ui_setuppluginsdialog.h"

class PluginsFilterProxyModel :
	public QSortFilterProxyModel
{
	Q_OBJECT;
public:
	PluginsFilterProxyModel(QObject *AParent = NULL);
	void setErrorsOnly(bool AErrors);
	void setDisabledOnly(bool ADisabled);
protected:
	bool filterAcceptsRow(int ASourceRow, const QModelIndex &ASourceParent) const;
private:
	bool FErrorsOnly;
	bool FDisableOnly;
};

class SetupPluginsDialog :
	public QDialog
{
	Q_OBJECT;
public:
	SetupPluginsDialog(IPluginManager *APluginManager, QDomDocument APluginsSetup, QWidget *AParent = NULL);
	~SetupPluginsDialog();
protected:
	void updatePlugins();
	void saveSettings();
protected slots:
	void onCurrentPluginChanged(const QModelIndex &ACurrent, const QModelIndex &APrevious);
	void onDialogButtonClicked(QAbstractButton *AButton);
	void onHomePageLinkActivated(const QString &ALink);
	void onDependsLinkActivated(const QString &ALink);
	void onSearchLineEditSearchStart();
	void onDisabledCheckButtonClicked();
	void onWithErrorsCheckButtonClicked();
private:
	Ui::SetupPluginsDialogClass ui;
private:
	IPluginManager *FPluginManager;
private:
	QDomDocument FPluginsSetup;
	QStandardItemModel FModel;
	PluginsFilterProxyModel FProxy;
	QMap<QUuid, QStandardItem *> FPluginItem;
	QMap<QStandardItem *, QDomElement> FItemElement;
};

#endif // SETUPPLUGINSDIALOG_H
