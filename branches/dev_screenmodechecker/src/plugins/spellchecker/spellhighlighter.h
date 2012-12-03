#ifndef SPELLHIGHLIGHTER_H
#define SPELLHIGHLIGHTER_H

#include <QString>
#include <QSyntaxHighlighter>

#include <interfaces/imultiuserchat.h>

class SpellHighlighter : 
	public QSyntaxHighlighter
{
public:
	SpellHighlighter(QTextDocument *ADocument, IMultiUserChat *AMultiUserChat);
	void setEnabled(bool AEnabled);
	virtual void highlightBlock(const QString &AText);
protected:
	inline bool isUserNickName(const QString &AText);
private:
	bool FEnabled;
	IMultiUserChat *FMultiUserChat;
	QTextCharFormat FCharFormat;
};

#endif //SPELLHIGHLIGHTER_H
