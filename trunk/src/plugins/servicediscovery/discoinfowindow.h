#ifndef DISCOINFOWINDOW_H
#define DISCOINFOWINDOW_H

#include <QDialog>
#include <interfaces/iservicediscovery.h>
#include "ui_discoinfowindow.h"

class DiscoInfoWindow :
	public QDialog
{
	Q_OBJECT;
public:
	DiscoInfoWindow(IServiceDiscovery *ADiscovery, const Jid &AStreamJid, const Jid &AContactJid, const QString &ANode, QWidget *AParent = NULL);
	~DiscoInfoWindow();
	virtual Jid streamJid() const;
	virtual Jid contactJid() const;
	virtual QString node() const;
protected:
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
