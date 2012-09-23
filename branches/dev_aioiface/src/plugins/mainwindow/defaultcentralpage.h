#ifndef DEFAULTCENTRALPAGE_H
#define DEFAULTCENTRALPAGE_H

#include <QLabel>
#include <interfaces/imainwindow.h>

class DefaultCentralPage : 
	public QLabel,
	public IMainCentralPage
{
	Q_OBJECT;
	Q_INTERFACES(IMainCentralPage);
public:
	DefaultCentralPage(QWidget *AParent);
	~DefaultCentralPage();
	virtual QWidget *instance() { return this; }
	virtual void showCentralPage(bool AMinimized = false);
	virtual QIcon centralPageIcon() const;
	virtual QString centralPageCaption() const;
signals:
	void centralPageShow(bool AMinimized);
	void centralPageChanged();
	void centralPageDestroyed();
};

#endif // DEFAULTCENTRALPAGE_H
