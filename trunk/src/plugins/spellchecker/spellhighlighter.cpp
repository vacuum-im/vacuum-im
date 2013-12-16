#include "spellhighlighter.h"
#include "spellchecker.h"
#include "spellbackend.h"

SpellHighlighter::SpellHighlighter(QTextDocument *ADocument, IMultiUserChat *AMultiUserChat) : QSyntaxHighlighter(ADocument)
{
	FEnabled = true;
	FMultiUserChat = AMultiUserChat;
	FCharFormat.setUnderlineColor(Qt::red);
	FCharFormat.setUnderlineStyle(QTextCharFormat::SpellCheckUnderline);
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
		// Match words (minimally) excluding digits within a word
		static const QRegExp expression("\\b[^\\s\\d]+\\b");

		int index = 0;
		while ((index = expression.indexIn(AText, index)) != -1)
		{
			int length = expression.matchedLength();
			if (!isUserNickName(expression.cap()))
			{
				if (!SpellBackend::instance()->isCorrect(expression.cap()))
				{
					setFormat(index, length, FCharFormat);
				}
			}
			index += length;
		}
	}
}

bool SpellHighlighter::isUserNickName(const QString &AText)
{
	return FMultiUserChat != NULL && FMultiUserChat->userByNick(AText) != NULL;
}
