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
    virtual void highlightBlock(const QString &AText);
    inline bool isUserNickName(const QString &AText);
private:
    IMultiUserChat *FMultiUserChat;
    QTextCharFormat FCharFormat;
};

#endif //SPELLHIGHLIGHTER_H
