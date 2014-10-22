#ifndef ARCHIVEVIEWTREEVIEW_H
#define ARCHIVEVIEWTREEVIEW_H

#include <QTreeView>

class ArchiveViewTreeView : 
	public QTreeView
{
	Q_OBJECT;
public:
	ArchiveViewTreeView(QWidget *AParent = NULL);
	~ArchiveViewTreeView();
	void setBottomWidget(QWidget *AWidget);
protected:
	void updateViewportMargins();
protected:
	void viewportEvent();
	void resizeEvent(QResizeEvent *AEvent);
private:
	QWidget *FWidget;
};

#endif // ARCHIVEVIEWTREEVIEW_H
