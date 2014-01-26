#ifndef MESSAGEEDITOR_H
#define MESSAGEEDITOR_H

#include <QTextEdit>

class MessageEditor :
			public QTextEdit
{
	Q_OBJECT;
public:
	MessageEditor(QWidget *AParent);
	~MessageEditor();
public:
	bool autoResize() const;
	void setAutoResize(bool AResize);
	int minimumLines() const;
	void setMinimumLines(int ALines);
signals:
	void createDataRequest(QMimeData *ADestination) const;
	void canInsertDataRequest(const QMimeData *AData, bool &ACanInsert) const;
	void insertDataRequest(const QMimeData *AData, QTextDocument *ADocument) const;
public:
	virtual QSize sizeHint() const;
	virtual QSize minimumSizeHint() const;
protected:
	int textHeight(int ALines = 0) const;
protected:
	virtual QMimeData *createMimeDataFromSelection() const;
	virtual bool canInsertFromMimeData(const QMimeData *ASource) const;
	virtual void insertFromMimeData(const QMimeData *ASource);
protected slots:
	void onTextChanged();
private:
	bool FAutoResize;
	int FMinimumLines;
};

#endif // MESSAGEEDITOR_H
