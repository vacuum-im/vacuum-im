#include "styleviewer.h"

#include <QApplication>
#include <QClipboard>
#include <QEvent>
#include <QHelpEvent>
#include <QKeyEvent>
#include <QMimeData>
#include <QMouseEvent>
#include <QToolTip>

#include <utils/textmanager.h>

StyleViewer::StyleViewer(QWidget *AParent) : AnimatedTextBrowser(AParent)
{
	setAnimated(true);
	setOpenLinks(false);
	setAcceptDrops(false);
	setUndoRedoEnabled(false);
	setOpenExternalLinks(false);
	setFrameShape(QFrame::NoFrame);
	setContextMenuPolicy(Qt::CustomContextMenu);
	// The links are opened by the message widgets, through the url handlers of
	// the plugins. Opening them here as well would open every link twice.
}

StyleViewer::~StyleViewer()
{

}

void StyleViewer::mousePressEvent(QMouseEvent *AEvent)
{
	QTextBrowser::mousePressEvent(AEvent);
}

void StyleViewer::keyPressEvent(QKeyEvent *AEvent)
{
	if (AEvent->matches(QKeySequence::Copy))
	{
		// The text of the links of the images is shortened for the display, the
		// full urls are copied to the clipboard instead of them.
		const QTextCursor cursor = textCursor();
		if (cursor.hasSelection())
		{
			QMimeData *data = new QMimeData;
			data->setHtml(cursor.selection().toHtml());
			data->setText(TextManager::getCopyableText(cursor.selection()));
			QApplication::clipboard()->setMimeData(data);
		}
		return;
	}
	AnimatedTextBrowser::keyPressEvent(AEvent);
}

bool StyleViewer::viewportEvent(QEvent *AEvent)
{
	if (AEvent->type() == QEvent::ToolTip)
	{
		QHelpEvent *help = static_cast<QHelpEvent *>(AEvent);
		const QTextCursor cursor = cursorForPosition(help->pos());
		const QTextCharFormat format = cursor.charFormat();
		if (format.isImageFormat() && format.isAnchor())
		{
			// The images of the gallery are links to the original images, the
			// name of the file is shown as the tooltip.
			const QUrl url(format.anchorHref());
			const QString path = url.path();
			const int slash = path.lastIndexOf(QLatin1Char('/'));
			if (slash >= 0 && slash + 1 < path.length())
			{
				QToolTip::showText(help->globalPos(), path.mid(slash + 1), this);
				return true;
			}
		}
	}
	return AnimatedTextBrowser::viewportEvent(AEvent);
}

QSize StyleViewer::sizeHint() const
{
	return QSize(256,192);
}

QSize StyleViewer::minimumSizeHint() const
{
	return QSize(70,50);
}
