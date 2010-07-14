#ifndef SETUPPLUGINSDIALOG_H
#define SETUPPLUGINSDIALOG_H

#include <QDialog>
#include <QDomDocument>
#include <definitions/menuicons.h>
#include <definitions/resources.h>
#include <interfaces/ipluginmanager.h>
#include <utils/iconstorage.h>
#include "ui_setuppluginsdialog.h"

class SetupPluginsDialog :
			public QDialog
{
	Q_OBJECT;
public:
	SetupPluginsDialog(IPluginManager *APluginManager, QDomDocument APluginsSetup, QWidget *AParent = NULL);
	~SetupPluginsDialog();
protected:
	void updateLanguage();
	void updatePlugins();
	void saveSettings();
	QDomElement getPluginElement(const QUuid &AUuid) const;
protected slots:
	void onCurrentLanguageChanged(int AIndex);
	void onCurrentPluginChanged(QTableWidgetItem *ACurrent, QTableWidgetItem *APrevious);
	void onDialogButtonClicked(QAbstractButton *AButton);
	void onHomePageLinkActivated(const QString &ALink);
private:
	Ui::SetupPluginsDialogClass ui;
private:
	IPluginManager *FPluginManager;
private:
	QDomDocument FPluginsSetup;
	QMap<QTableWidgetItem *, QDomElement> FItemElement;
};

#endif // SETUPPLUGINSDIALOG_H
