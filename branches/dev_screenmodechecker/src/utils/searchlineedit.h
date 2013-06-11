#ifndef SEARCHLINEEDIT_H
#define SEARCHLINEEDIT_H

#include <QTimer>
#include <QLineEdit>
#include <QToolButton>
#include "utilsexport.h"
#include "menu.h"
#include "closebutton.h"

class UTILS_EXPORT SearchLineEdit : 
	public QLineEdit
{
	Q_OBJECT;
public:
	SearchLineEdit(QWidget *AParent = NULL);
	~SearchLineEdit();
	Menu *searchMenu() const;
	CloseButton *clearButton() const;
	bool isStartingSearch() const;
	void restartTimeout(int AMSecs);
	int startSearchTimeout() const;
	void setStartSearchTimeout(int AMSecs);
	bool isSearchMenuVisible() const;
	void setSearchMenuVisible(bool AVisible);
	bool isSelectTextOnFocusEnabled() const;
	void setSelectTextOnFocusEnabled(bool AEnabled);
signals:
	void searchStart();
	void searchNext();
protected:
	bool event(QEvent *AEvent);
	void showEvent(QShowEvent *AEvent);
private:
	void updateTextMargins();
	void updateLayoutMargins();
private slots:
	void onStartTimerTimeout();
	void onClearButtonClicked();
	void onLineEditReturnPressed();
	void onLineEditTextChanged(const QString &AText);
private:
	bool FSelectText;
	bool FTextChanged;
	int FStartTimeout;
	Menu *FSearchMenu;
	QTimer FStartTimer;
	QToolButton *FMenuButton;
	CloseButton *FClearButton;
};

#endif // SEARCHLINEEDIT_H
