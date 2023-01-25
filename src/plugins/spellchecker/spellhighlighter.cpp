#include "spellhighlighter.h"

SpellHighlighter::SpellHighlighter(ISpellChecker *ASpellChecker, QTextDocument *ADocument, IMultiUserChat *AMultiUserChat) : QSyntaxHighlighter(ADocument)
{
	FEnabled = true;
	FSpellChecker = ASpellChecker;
	FMultiUserChat = AMultiUserChat;
	FCharFormat.setUnderlineColor(Qt::red);
	FCharFormat.setUnderlineStyle(QTextCharFormat::SpellCheckUnderline);
	// R"(\b[^\s\d]+\b)"
	// R"([^\W\d][\w]+)"
	FWordsExpr = QRegularExpression(R"(\b(?!\W)[^\W\d][\w]+)", QRegularExpression::UseUnicodePropertiesOption);
}

void SpellHighlighter::setEnabled(bool AEnabled)
{
	if (FEnabled != AEnabled)
	{
		FEnabled = AEnabled;
		rehighlight();
	}
}

void SpellHighlighter::highlightBlock(const QString &AText)
{
	if (FEnabled)
	{
		QRegularExpressionMatchIterator iter = FWordsExpr.globalMatch(AText);
		while(iter.hasNext())
		{
			const QRegularExpressionMatch match = iter.next();
			if (!FSpellChecker->isCorrectWord(match.captured()))
				setFormat(match.capturedStart(), match.capturedLength(), FCharFormat);
		}
	}
}

bool SpellHighlighter::isUserNickName(const QString &AText)
{
	return FMultiUserChat!=NULL && FMultiUserChat->findUser(AText)!=NULL;
}
