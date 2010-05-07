#ifndef DATAMEDIAWIDGET_H
#define DATAMEDIAWIDGET_H

#include <QLabel>
#include <QVBoxLayout>
#include <interfaces/idataforms.h>

class DataMediaWidget :
			public QLabel,
			public IDataMediaWidget
{
	Q_OBJECT;
	Q_INTERFACES(IDataMediaWidget);
public:
	DataMediaWidget(IDataForms *ADataForms, const IDataMedia &AMedia, QWidget *AParent);
	~DataMediaWidget();
	virtual QWidget *instance() { return this; }
	virtual IDataMedia media() const;
	virtual IDataMediaURI mediaUri() const;
signals:
	void mediaShown();
	void mediaError(const QString &AError);
protected:
	void loadUri();
	bool updateWidget(const IDataMediaURI &AUri, const QByteArray &AData);
protected slots:
	void onUrlLoaded(const QUrl &AUrl, const QByteArray &AData);
	void onUrlLoadFailed(const QUrl &AUrl, const QString &AError);
private:
	IDataForms *FDataForms;
private:
	int uriIndex;
	IDataMedia FMedia;
	QString FLastError;
};

#endif // DATAMEDIAWIDGET_H
