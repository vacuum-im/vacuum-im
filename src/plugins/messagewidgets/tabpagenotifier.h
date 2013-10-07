#ifndef TABPAGENOTIFIER_H
#define TABPAGENOTIFIER_H

#include <QMultiMap>
#include <interfaces/imessagewidgets.h>

class TabPageNotifier : 
	public QObject,
	public ITabPageNotifier
{
	Q_OBJECT;
	Q_INTERFACES(ITabPageNotifier);
public:
	TabPageNotifier(ITabPage *ATabPage);
	virtual ~TabPageNotifier();
	virtual QObject *instance() { return this; }
	virtual ITabPage *tabPage() const;
	virtual int activeNotify() const;
	virtual QList<int> notifies() const;
	virtual ITabPageNotify notifyById(int ANotifyId) const;
	virtual int insertNotify(const ITabPageNotify &ANotify);
	virtual void removeNotify(int ANotifyId);
signals:
	void notifyInserted(int ANotifyId);
	void notifyRemoved(int ANotifyId);
	void activeNotifyChanged(int ANotifyId);
protected slots:
	void onUpdateTimerTimeout();
private:
	ITabPage *FTabPage;
private:
	int FActiveNotify;
	QTimer FUpdateTimer;
	QMap<int, ITabPageNotify> FNotifies;
	QMultiMap<int, int> FNotifyIdByPriority;
};

#endif // TABPAGENOTIFIER_H
