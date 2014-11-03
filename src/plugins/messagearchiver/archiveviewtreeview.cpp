#include "archiveviewtreeview.h"

#include <QPainter>

ArchiveViewTreeView::ArchiveViewTreeView(QWidget *AParent) : QTreeView(AParent)
{
	FWidget = NULL;
}

ArchiveViewTreeView::~ArchiveViewTreeView()
{

}

void ArchiveViewTreeView::setBottomWidget(QWidget *AWidget)
{
	FWidget = AWidget;
	updateViewportMargins();
}

void ArchiveViewTreeView::updateViewportMargins()
{
	if (FWidget)
		setViewportMargins(0,0,0,height()-FWidget->geometry().top()-1);
}

void ArchiveViewTreeView::resizeEvent(QResizeEvent *AEvent)
{
	QTreeView::resizeEvent(AEvent);
	updateViewportMargins();
}
