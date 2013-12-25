#ifndef SOCKSOPTIONS_H
#define SOCKSOPTIONS_H

#include <QWidget>
#include <interfaces/isocksstreams.h>
#include <interfaces/iconnectionmanager.h>
#include <interfaces/ioptionsmanager.h>
#include <utils/options.h>
#include "ui_socksoptions.h"

class SocksOptions :
	public QWidget,
	public IOptionsWidget
{
	Q_OBJECT;
	Q_INTERFACES(IOptionsWidget);
public:
	SocksOptions(ISocksStreams *ASocksStreams, ISocksStream *ASocksStream, bool AReadOnly, QWidget *AParent = NULL);
	SocksOptions(ISocksStreams *ASocksStreams, IConnectionManager *AConnectionManager, const OptionsNode &ANode, bool AReadOnly, QWidget *AParent = NULL);
	~SocksOptions();
	virtual QWidget *instance() { return this; }
public slots:
	void apply(OptionsNode ANode);
	void apply(ISocksStream *AStream);
	virtual void apply();
	virtual void reset();
signals:
	void modified();
	void childApply();
	void childReset();
protected:
	void initialize(bool AReadOnly);
protected slots:
	void onAddStreamProxyClicked(bool);
	void onStreamProxyUpClicked(bool);
	void onStreamProxyDownClicked(bool);
	void onDeleteStreamProxyClicked(bool);
private:
	Ui::SocksOptionsClass ui;
private:
	ISocksStreams *FSocksStreams;
	IConnectionManager *FConnectionManager;
private:
	OptionsNode FOptions;
	ISocksStream *FSocksStream;
	IOptionsWidget *FProxySettings;
};

#endif // SOCKSOPTIONS_H
