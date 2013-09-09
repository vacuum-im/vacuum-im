#ifndef CLOSEBUTTON_H
#define CLOSEBUTTON_H

#include <QAbstractButton>
#include "utilsexport.h"

class UTILS_EXPORT CloseButton : 
	public QAbstractButton
{
	Q_OBJECT;
public:
	CloseButton(QWidget *AParent = NULL);
	~CloseButton();
	QSize sizeHint() const;
protected:
	void enterEvent(QEvent *AEvent);
	void leaveEvent(QEvent *AEvent);
	void paintEvent(QPaintEvent *AEvent);
};

#endif // CLOSEBUTTON_H
