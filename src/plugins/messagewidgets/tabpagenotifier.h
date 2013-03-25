#ifndef TABPAGENOTIFIER_H
#define TABPAGENOTIFIER_H

#include <QMultiMap>
#include <interfaces/imessagewidgets.h>

class TabPageNotifier : 
	public QObject,
	public IMessageTabPageNotifier
{
	Q_OBJECT;
	Q_INTERFACES(IMessageTabPageNotifier);
public:
	TabPageNotifier(IMessageTabPage *ATabPage);
	virtual ~TabPageNotifier();
	virtual QObject *instance() { return this; }
	virtual IMessageTabPage *tabPage() const;
	virtual int activeNotify() const;
	virtual QList<int> notifies() const;
	virtual IMessageTabPageNotify notifyById(int ANotifyId) const;
	virtual int insertNotify(const IMessageTabPageNotify &ANotify);
	virtual void removeNotify(int ANotifyId);
signals:
	void notifyInserted(int ANotifyId);
	void notifyRemoved(int ANotifyId);
	void activeNotifyChanged(int ANotifyId);
protected slots:
	void onUpdateTimerTimeout();
private:
	IMessageTabPage *FTabPage;
private:
	int FActiveNotify;
	QTimer FUpdateTimer;
	QMap<int, IMessageTabPageNotify> FNotifies;
	QMultiMap<int, int> FNotifyIdByPriority;
};

#endif // TABPAGENOTIFIER_H
