#ifndef ARCHIVEVIEWWINDOW_H
#define ARCHIVEVIEWWINDOW_H

#include <QMainWindow>
#include <QStandardItemModel>
#include <definitions/menuicons.h>
#include <definitions/resources.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/imessagearchiver.h>
#include <interfaces/iroster.h>
#include <interfaces/istatusicons.h>
#include <interfaces/imessagestyles.h>
#include <interfaces/imessagewidgets.h>
#include "ui_archiveviewwindow.h"

class ArchiveViewWindow : 
	public QMainWindow
{
	Q_OBJECT;
public:
	ArchiveViewWindow(IPluginManager *APluginManager, IMessageArchiver *AArchiver, IRoster *ARoster, QWidget *AParent = NULL);
	~ArchiveViewWindow();
	Jid streamJid() const;
	Jid contactJid() const;
	void setContactJid(const Jid &AContactJid);
protected:
	void initialize(IPluginManager *APluginManager);
private:
	Ui::ArchiveViewWindow ui;
private:
	IRoster *FRoster;
	IMessageArchiver *FArchiver;
	IStatusIcons *FStatusIcons;
	IMessageStyles *FMessageStyles;
	IMessageWidgets *FMessageWidgets;
private:
	IViewWidget *FViewWidget;
	QStandardItemModel *FModel;
	QMap<IArchiveHeader,IArchiveCollection> FCollections;
};

#endif // ARCHIVEVIEWWINDOW_H
