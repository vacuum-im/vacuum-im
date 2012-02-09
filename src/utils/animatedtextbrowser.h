#ifndef ANIMATEDTEXTBROWSER_H
#define ANIMATEDTEXTBROWSER_H

#include <QMovie>
#include <QTextBrowser>
#include <QNetworkAccessManager>
#include "utilsexport.h"

class UTILS_EXPORT AnimatedTextBrowser : 
	public QTextBrowser
{
	Q_OBJECT;
public:
	AnimatedTextBrowser(QWidget *AParent = NULL);
	bool isAnimated();
	void setAnimated(bool AAnimated);    
	QPixmap addAnimation(const QUrl& AUrl, const QVariant &AImageData);
	QNetworkAccessManager *networkAccessManager() const;
	void setNetworkAccessManager(QNetworkAccessManager *ANetworkAccessManager);
signals:
	void resourceAdded(QUrl AName);
	void resourceLoaded(QUrl AName);
protected:
	virtual QVariant loadResource(int AType, const QUrl &AName);
protected slots:
	void onAnimationFrameChanged();
	void onResourceLoadFinished();    
private:
	bool FBusy;
	bool FAnimated;
	QHash<QMovie*, QUrl> FUrls;
	QHash<QString, QVariant> FElements;
	QNetworkAccessManager *FNetworkAccessManager;
};

#endif // ANIMATEDTEXTBROWSER_H
