#ifndef EMOTICONSOPTIONS_H
#define EMOTICONSOPTIONS_H

#include <QWidget>
#include <interfaces/iemoticons.h>
#include <interfaces/ioptionsmanager.h>
#include "ui_emoticonsoptions.h"

class EmoticonsOptions :
	public QWidget,
	public IOptionsWidget
{
	Q_OBJECT;
	Q_INTERFACES(IOptionsWidget);
public:
	EmoticonsOptions(IEmoticons *AEmoticons, QWidget *AParent);
	~EmoticonsOptions();
	virtual QWidget* instance() { return this; }
public slots:
	virtual void apply();
	virtual void reset();
signals:
	void modified();
	void childApply();
	void childReset();
protected slots:
	void onUpButtonClicked();
	void onDownButtonClicked();
private:
	Ui::EmoticonsOptionsClass ui;
private:
	IEmoticons *FEmoticons;
};

#endif // EMOTICONSOPTIONS_H
