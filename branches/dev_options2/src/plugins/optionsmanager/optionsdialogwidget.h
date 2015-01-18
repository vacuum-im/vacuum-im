#ifndef OPTIONSWIDGET_H
#define OPTIONSWIDGET_H

#include <QLabel>
#include <QDateEdit>
#include <QTimeEdit>
#include <QCheckBox>
#include <QLineEdit>
#include <QComboBox>
#include <QFontComboBox>
#include <QDateTimeEdit>
#include <QHBoxLayout>
#include <interfaces/ioptionsmanager.h>

class OptionsDialogWidget :
	public QWidget,
	public IOptionsDialogWidget
{
	Q_OBJECT;
	Q_INTERFACES(IOptionsDialogWidget);
public:
	OptionsDialogWidget(const OptionsNode &ANode, const QString &ACaption, QWidget *AParent);
	~OptionsDialogWidget();
	virtual QWidget* instance() { return this; }
public slots:
	virtual void apply();
	virtual void reset();
signals:
	void modified();
	void childApply();
	void childReset();
protected:
	void insertWithCaption(const QString &ACaption, QWidget *ABuddy, QHBoxLayout *ALayout);
	bool eventFilter(QObject *AWatched, QEvent *AEvent);
private:
	QLabel *FLabel;
	QCheckBox *FCheckBox;
	QLineEdit *FLineEdit;
	QComboBox *FComboBox;
	QFontComboBox *FFontComboBox;
	QDateTimeEdit *FDateTimeEdit;
private:
	QVariant FValue;
	OptionsNode FNode;
};

#endif // OPTIONSWIDGET_H
