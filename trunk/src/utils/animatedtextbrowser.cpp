#include "animatedtextbrowser.h"

#include <QBuffer>
#include <QTextLayout>
#include <QNetworkReply>

AnimatedTextBrowser::AnimatedTextBrowser(QWidget *AParent) : QTextBrowser(AParent)
{
	FBusy = false;
	FAnimated = false;
	FNetworkAccessManager = NULL;
}

bool AnimatedTextBrowser::isAnimated()
{
	return FAnimated;
}

void AnimatedTextBrowser::setAnimated(bool AAnimated)
{
	if (FAnimated != AAnimated)
	{
		FAnimated = AAnimated;
		foreach(QMovie *movie, FUrls.keys())
		{
			if (!FAnimated)
				movie->jumpToFrame(0);
			movie->setPaused(!FAnimated);
		}
	}
}

QPixmap AnimatedTextBrowser::addAnimation(const QUrl& AUrl, const QVariant &AImageData)
{
	QMovie *movie = NULL;
	switch (AImageData.type())
	{
	case QVariant::String:
		{
			movie = new QMovie(AImageData.toString(), QByteArray(), this);
			break;
		}
	case QVariant::ByteArray:
		{
			QBuffer *buffer = new QBuffer();
			buffer->setData(AImageData.toByteArray());
			buffer->open(QBuffer::ReadOnly);
			movie = new QMovie(buffer, QByteArray(), this);
			buffer->setParent(movie);
			break;
		}
	default:
		return QPixmap(); // Nothing to do
	}
	
	FUrls.insert(movie, AUrl);
	connect(movie, SIGNAL(frameChanged(int)), SLOT(onAnimationFrameChanged()));
	
	movie->start();
	movie->setPaused(!FAnimated);

	return movie->currentPixmap();
}

QNetworkAccessManager *AnimatedTextBrowser::networkAccessManager() const
{
	return FNetworkAccessManager;
}

void AnimatedTextBrowser::setNetworkAccessManager(QNetworkAccessManager *ANetworkAccessManager)
{
	FNetworkAccessManager=ANetworkAccessManager;
}

QVariant AnimatedTextBrowser::loadResource(int AType, const QUrl &AName)
{
	FBusy = true;   // Set busy flag to prevent crash

	QVariant result;
	QString nameString = AName.toString();

	if (!FElements.contains(nameString))                     // Resource is not loaded yet
	{
		result = QTextBrowser::loadResource(AType, AName);    // Try default resource loader
		if (FNetworkAccessManager && result.isNull())         // If got nothing Try using Network Access Manager if available
		{
			FElements.insert(nameString, QVariant());    // Mark resource as "loading" by inserting an empty QVariant into hash table
			QNetworkReply *reply = FNetworkAccessManager->get(QNetworkRequest(AName));   // Start loading resource
			connect(reply, SIGNAL(finished()), SLOT(onResourceLoadFinished()));
		}
	}
	else
	{
		result = FElements.value(nameString);
	}

	if (!result.isNull())
		result=addAnimation(AName, result.toByteArray());

	FBusy = false;
	return result;
}

void AnimatedTextBrowser::onAnimationFrameChanged()
{
	if (!FBusy)   // Skip frame if currently busy
	{
		QMovie* movie = qobject_cast<QMovie *>(sender());
		if (movie)
		{
			QUrl url = FUrls.value(movie);
			document()->addResource(QTextDocument::ImageResource, url, movie->currentPixmap());
			emit resourceAdded(url);
			setLineWrapColumnOrWidth(lineWrapColumnOrWidth()); // causes reload
		}
	}
}

void AnimatedTextBrowser::onResourceLoadFinished()
{
	QNetworkReply *reply=qobject_cast<QNetworkReply *>(sender());
	if (reply->error() == QNetworkReply::NoError)
	{
		QByteArray data = reply->readAll();
		FElements.insert(reply->url().toString(), data);
		emit resourceLoaded(reply->url());
		setLineWrapColumnOrWidth(lineWrapColumnOrWidth()); // causes reload
	}
	reply->deleteLater();
}
