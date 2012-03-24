#ifndef ARCHIVEENGINESOPTIONS_H
#define ARCHIVEENGINESOPTIONS_H

#include <QGroupBox>
#include <QPushButton>
#include <interfaces/imessagearchiver.h>
#include <interfaces/ioptionsmanager.h>

class EngineWidget : 
	public QGroupBox
{
	Q_OBJECT;
public:
	EngineWidget(IMessageArchiver *AArchiver, IArchiveEngine *AEngine, QWidget *AParent);
	~EngineWidget();
	void apply();
	void reset();
signals:
	void modified();
protected:
	void setEngineState(bool AEnabled);
protected slots:
	void onEnableButtonClicked();
	void onDisableButtonClicked();
private:
	IArchiveEngine *FEngine;
	IMessageArchiver *FArchiver;
private:
	bool FEnabled;
	QPushButton *pbtEnable;
	QPushButton *pbtDisable;
};

class ArchiveEnginesOptions : 
	public QWidget,
	public IOptionsWidget
{
	Q_OBJECT;
	Q_INTERFACES(IOptionsWidget);
public:
	ArchiveEnginesOptions(IMessageArchiver *AArchiver, QWidget *AParent = NULL);
	~ArchiveEnginesOptions();
	virtual QWidget* instance() { return this; }
public slots:
	virtual void apply();
	virtual void reset();
signals:
	void modified();
	void childApply();
	void childReset();
private:
	IMessageArchiver *FArchiver;
private:
	QMap<IArchiveEngine *, EngineWidget *> FEngines;
};

#endif // ARCHIVEENGINESOPTIONS_H
