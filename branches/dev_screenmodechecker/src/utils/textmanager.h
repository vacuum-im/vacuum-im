#ifndef TEXTMANAGER_H
#define TEXTMANAGER_H

#include <QTextCursor>
#include <QTextDocument>
#include <QTextDocumentFragment>
#include "utilsexport.h"

class UTILS_EXPORT TextManager
{
public:
	static QString getDocumentBody(const QTextDocument &ADocument);
	static QString getTextFragmentHref(const QTextDocumentFragment &AFragment);
	static void insertQuotedFragment(QTextCursor ACursor, const QTextDocumentFragment &AFragment);
	static QTextDocumentFragment getTrimmedTextFragment(const QTextDocumentFragment &AFragment, bool APlainText = false);
	static QString getElidedString(const QString &AString, Qt::TextElideMode AMode, int AMaxChars);
private:
	TextManager();
};

#endif // TEXTMANAGER_H
