#ifndef SPELLHIGHLIGHTER_H
#define SPELLHIGHLIGHTER_H

#include <QString>
#include <QSyntaxHighlighter>
#include <QRegularExpression>
#include <interfaces/imultiuserchat.h>
#include <interfaces/ispellchecker.h>

class SpellHighlighter : public QSyntaxHighlighter
{
	Q_OBJECT
public:
	SpellHighlighter(ISpellChecker *ASpellChecker, QTextDocument *ADocument, IMultiUserChat *AMultiUserChat);
	void setEnabled(bool AEnabled);
protected:
	void highlightBlock(const QString &text) override;
	inline bool isUserNickName(const QString &AText);

private:
	bool FEnabled;
	QRegularExpression FWordsExpr;
	IMultiUserChat *FMultiUserChat;
	ISpellChecker *FSpellChecker;
	QTextCharFormat FCharFormat;
};

#endif //SPELLHIGHLIGHTER_H
