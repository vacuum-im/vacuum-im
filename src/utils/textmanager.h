#ifndef TEXTMANAGER_H
#define TEXTMANAGER_H

#include <QTextCursor>
#include <QTextDocument>
#include <QTextDocumentFragment>
#include <QTextFormat>
#include "utilsexport.h"

// The property of the links of the images: it keeps the full url of the image,
// the text of the link is shortened for the display, but the full url is
// copied to the clipboard.
#define TMI_IMAGE_LINK_URL (QTextFormat::UserProperty + 1)

class UTILS_EXPORT TextManager
{
public:
	static QString getDocumentBody(const QTextDocument &ADocument);
	static QString getTextFragmentHref(const QTextDocumentFragment &AFragment);
	static void insertQuotedFragment(QTextCursor ACursor, const QTextDocumentFragment &AFragment);
	static QTextDocumentFragment getTrimmedTextFragment(const QTextDocumentFragment &AFragment, bool APlainText = false);
	static QString getElidedString(const QString &AString, Qt::TextElideMode AMode, int AMaxChars);
	// The text of the fragment for the clipboard: the full urls are used for the
	// links of the images, their text is shortened for the display only
	static QString getCopyableText(const QTextDocumentFragment &AFragment);
private:
	TextManager();
};

#endif // TEXTMANAGER_H
