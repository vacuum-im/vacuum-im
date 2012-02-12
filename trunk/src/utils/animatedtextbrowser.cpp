#include "animatedtextbrowser.h"

#include <QBuffer>
#include <QTextBlock>
#include <QTextLayout>
#include <QNetworkReply>

AnimatedTextBrowser::AnimatedTextBrowser(QWidget *AParent) : QTextBrowser(AParent)
{
	FAnimated = false;
	FNetworkAccessManager = NULL;

	FUpdateTimer.setSingleShot(true);
	connect(&FUpdateTimer,SIGNAL(timeout()),SLOT(onUpdateDocumentAnimation()));

	connect(document(),SIGNAL(contentsChange(int, int, int)),SLOT(onDocumentContentsChanged(int,int,int)));
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
			FChangedMovies += movie;
		}
		FUpdateTimer.start(0);
	}
}

QNetworkAccessManager *AnimatedTextBrowser::networkAccessManager() const
{
	return FNetworkAccessManager;
}

void AnimatedTextBrowser::setNetworkAccessManager(QNetworkAccessManager *ANetworkAccessManager)
{
	FNetworkAccessManager=ANetworkAccessManager;
}

QList<int> AnimatedTextBrowser::findUrlPositions(const QUrl &AName) const
{
	QList<int> positions;

	QTextBlock block = document()->firstBlock();
	while (block.isValid())
	{
		for (QTextBlock::iterator it = block.begin(); !it.atEnd(); it++)
		{
			QTextFragment fragment = it.fragment();
			if (fragment.charFormat().isImageFormat())
			{
				if (AName == fragment.charFormat().toImageFormat().name())
					positions.append(fragment.position());
			}
		}
		block = block.next();
	}

	return positions;
}

QPixmap AnimatedTextBrowser::addAnimation(const QUrl &AName, const QVariant &AImageData)
{
	QMovie *movie = FUrls.key(AName);
	if (movie == NULL)
	{
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
			break;
		}

		if (movie != NULL)
		{
			FUrls.insert(movie, AName);
			FUrlPositions.insert(movie,findUrlPositions(AName));
			connect(movie,SIGNAL(frameChanged(int)),SLOT(onAnimationFrameChanged()));
			connect(movie,SIGNAL(destroyed(QObject *)),SLOT(onMovieDestroyed(QObject *)));

			movie->start();
			movie->setPaused(!FAnimated);
		}
	}
	else
	{
		FUrlPositions[movie] = findUrlPositions(AName);
	}
	return movie!=NULL ? movie->currentPixmap() : QPixmap();
}

void AnimatedTextBrowser::showEvent(QShowEvent *AEvent)
{
	FUpdateTimer.start(0);
	QTextBrowser::showEvent(AEvent);
}

QVariant AnimatedTextBrowser::loadResource(int AType, const QUrl &AName)
{
	if (AType == QTextDocument::ImageResource)
	{
		QVariant result;
		QString nameString = AName.toString();

		if (!FResources.contains(nameString))                   // Resource is not loaded yet
		{
			result = QTextBrowser::loadResource(AType, AName);    // Try default resource loader
			if (FNetworkAccessManager && result.isNull())         // If got nothing Try using Network Access Manager if available
			{
				FResources.insert(nameString, QVariant());          // Mark resource as "loading" by inserting an empty QVariant into hash table
				QNetworkReply *reply = FNetworkAccessManager->get(QNetworkRequest(AName));   // Start loading resource
				connect(reply, SIGNAL(finished()), SLOT(onResourceLoadFinished()));
			}
		}
		else
		{
			result = FResources.value(nameString);
		}

		if (!result.isNull())
			result = addAnimation(AName, result.toByteArray());

		return result;
	}
	return QTextBrowser::loadResource(AType, AName);
}

void AnimatedTextBrowser::onAnimationFrameChanged()
{
	QMovie *movie = qobject_cast<QMovie *>(sender());
	if (movie)
	{
		FChangedMovies += movie;
		FUpdateTimer.start(0);
	}
}

void AnimatedTextBrowser::onResourceLoadFinished()
{
	QNetworkReply *reply=qobject_cast<QNetworkReply *>(sender());
	if (reply->error() == QNetworkReply::NoError)
	{
		QByteArray data = reply->readAll();
		FResources.insert(reply->url().toString(), data);
		emit resourceLoaded(reply->url());
		setLineWrapColumnOrWidth(lineWrapColumnOrWidth());
	}
	reply->deleteLater();
}

void AnimatedTextBrowser::onUpdateDocumentAnimation()
{
	if (isVisible())
	{
		static const int minUpdateTimeout = qRound(1000/15.0);
#if QT_VERSION >= QT_VERSION_CHECK(4,7,0)
		qint64 timeout = FLastUpdate.isValid() ? qAbs(FLastUpdate.msecsTo(QDateTime::currentDateTime())) : minUpdateTimeout;
#else
		qint64 timeout = FLastUpdate.isValid() ? qAbs(FLastUpdate.time().msecsTo(QDateTime::currentDateTime().time())) : minUpdateTimeout;
#endif
		if (timeout >= minUpdateTimeout)
		{
			QList<int> dirtyBlocks;
			document()->blockSignals(true);
			QTextCursor cursor(document());
			cursor.beginEditBlock();
			foreach(QMovie *movie, FChangedMovies)
			{
				QUrl url = FUrls.value(movie);
				document()->addResource(QTextDocument::ImageResource, url, movie->currentPixmap());

				foreach(int pos, FUrlPositions.value(movie))
				{
					cursor.setPosition(pos);
					int block = document()->findBlock(pos).blockNumber();
					if (!dirtyBlocks.contains(block))
					{
						cursor.movePosition(QTextCursor::NextCharacter,QTextCursor::KeepAnchor);
						cursor.insertImage(cursor.charFormat().toImageFormat());
						dirtyBlocks.append(block);
					}
				}
				emit resourceUpdated(url);
			}
			cursor.endEditBlock();
			document()->blockSignals(false);

			FChangedMovies.clear();
			FLastUpdate = QDateTime::currentDateTime();
		}
		else
		{
			FUpdateTimer.start(minUpdateTimeout-timeout+1);
		}
	}
}

void AnimatedTextBrowser::onMovieDestroyed(QObject *AObject)
{
	QMovie *movie = static_cast<QMovie *>(AObject);
	if (movie)
	{
		FChangedMovies -= movie;
		FUrls.remove(movie);
		FUrlPositions.remove(movie);
	}
}

void AnimatedTextBrowser::onDocumentContentsChanged(int APosition, int ARemoved, int AAdded)
{
	for(QHash<QMovie *, QList<int> >::iterator it=FUrlPositions.begin(); it!=FUrlPositions.end(); it++)
	{
		QList<int> &positions = it.value();
		for(int i=0; i<positions.count(); i++)
		{
			int pos = positions.at(i);
			if (pos >= APosition)
			{
				if (pos < APosition+ARemoved)
					positions.removeAt(i--);            // Position was removed // TODO: Destroy movie if positions is empty
				else
					positions[i] += AAdded-ARemoved;    // Updating image position
			}
		}
	}
}
