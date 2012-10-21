#ifndef DISCOINFOWINDOW_H
#define DISCOINFOWINDOW_H

#include <QDialog>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <interfaces/iservicediscovery.h>
#include <utils/menu.h>
#include "ui_discoinfowindow.h"

class DiscoInfoWindow :
			public QDialog
{
	Q_OBJECT;
public:
	DiscoInfoWindow(IServiceDiscovery *ADiscovery, const Jid &AStreamJid, const Jid &AContactJid,
	                const QString &ANode, QWidget *AParent = NULL);
	~DiscoInfoWindow();
	virtual Jid streamJid() const { return FStreamJid; }
	virtual Jid contactJid() const { return FContactJid; }
	virtual QString node() const { return FNode; }
protected:
	void initialize();
	void updateWindow();
	void requestDiscoInfo();
protected slots:
	void onDiscoInfoReceived(const IDiscoInfo &ADiscoInfo);
	void onCurrentFeatureChanged(QListWidgetItem *ACurrent, QListWidgetItem *APrevious);
	void onUpdateClicked();
	void onListItemDoubleClicked(QListWidgetItem *AItem);
	void onShowExtensionForm(bool);
private:
	Ui::DiscoInfoWindowClass ui;
private:
	IDataForms *FDataForms;
	IServiceDiscovery *FDiscovery;
private:
	Menu *FFormMenu;
	Jid FStreamJid;
	Jid FContactJid;
	QString FNode;
};

#endif // DISCOINFOWINDOW_H
