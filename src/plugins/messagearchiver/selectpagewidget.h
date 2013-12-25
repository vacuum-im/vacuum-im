#ifndef SELECTPAGEWIDGET_H
#define SELECTPAGEWIDGET_H

#include <QDate>
#include <QWidget>
#include "ui_selectpagewidget.h"

class SelectPageWidget : 
	public QWidget
{
	Q_OBJECT;
public:
	SelectPageWidget(QWidget *AParent = NULL);
	~SelectPageWidget();
	int monthShown() const;
	int yearShown() const;
public slots:
	void showNextMonth();
	void showPreviousMonth();
	void setCurrentPage(int AYear, int AMonth);
signals:
	void currentPageChanged(int AYear, int AMonth);
protected:
	bool eventFilter(QObject *AWatched, QEvent *AEvent);
protected slots:
	void onStartEditYear();
	void onChangeYearBySpinbox();
	void onChangeMonthByAction();
private:
	Ui::SelectPageWidgetClass ui;
private:
	int FYear;
	int FMonth;
	QLocale FLocale;
	Qt::FocusPolicy FOldFocusPolicy;
};

#endif // SELECTPAGEWIDGET_H
