#include "spellhighlighter.h"
#include "spellchecker.h"
#include "spellbackend.h"

SpellHighlighter::SpellHighlighter(QTextDocument *ADocument, IMultiUserChat *AMultiUserChat)
    : QSyntaxHighlighter(ADocument),
      FMultiUserChat(AMultiUserChat)
{
    FCharFormat.setUnderlineColor(Qt::red);
    FCharFormat.setUnderlineStyle(QTextCharFormat::SpellCheckUnderline);
}

void SpellHighlighter::highlightBlock(const QString &AText)
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

bool SpellHighlighter::isUserNickName(const QString &AText)
{
    return FMultiUserChat != NULL && FMultiUserChat->userByNick(AText) != NULL;
}
