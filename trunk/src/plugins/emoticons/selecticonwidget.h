#ifndef SELECTICONWIDGET_H
#define SELECTICONWIDGET_H

#include <QMap>
#include <QLabel>
#include <QEvent>
#include <QGridLayout>
#include <utils/iconstorage.h>

#include <QMainWindow>

class SelectIconWidget :
			public QWidget
{
	Q_OBJECT;
public:
	SelectIconWidget(IconStorage *AStorage, QWidget *AParent = NULL);
	~SelectIconWidget();
signals:
	void iconSelected(const QString &ASubStorage, const QString &AIconKey);
protected:
	void createLabels();
protected:
	virtual bool eventFilter(QObject *AWatched, QEvent *AEvent);
private:
	QLabel *FPressed;
	QGridLayout *FLayout;
	IconStorage *FStorage;
	QMap<QLabel *, QString> FKeyByLabel;
};

#endif // SELECTICONWIDGET_H
