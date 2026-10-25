#include "textmanager.h"

#include <QRegExp>
#include <QTextBlock>
#include <QTextCursor>

#include <algorithm>

TextManager::TextManager()
{

}

QString TextManager::getDocumentBody(const QTextDocument &ADocument)
{
	QRegExp body("<body.*>(.*)</body>");
	body.setMinimal(false);

	QString html = ADocument.toHtml();
	html = html.indexOf(body)>=0 ? body.cap(1).trimmed() : html;

	// XXX Replace <P> inserted by QTextDocument with <SPAN>
	if (html.leftRef(3).compare(QString("<p "), Qt::CaseInsensitive)==0 && html.rightRef(4).compare(QString("</p>"), Qt::CaseInsensitive)==0)
	{
		html.replace(1, 1, "span");
		html.replace(html.length() - 2, 1, "span");
	}

	return html;
}

QString TextManager::getTextFragmentHref(const QTextDocumentFragment &AFragment)
{
	if (AFragment.isEmpty())
		return QString();

	QString href;

	QTextDocument doc;
	QTextCursor cursor(&doc);
	cursor.insertFragment(AFragment);

	QTextBlock block = doc.firstBlock();
	while (block.isValid())
	{
		for (QTextBlock::iterator it = block.begin(); !it.atEnd(); ++it)
		{
			const QTextCharFormat format = it.fragment().charFormat();

			// ImageViewer stores the original (non-shortened) URL in a custom
			// format property so that copying the link retrieves the full URL
			// instead of the shortened visible text.
			QString fragmentHref = format.property(TMI_IMAGE_LINK_URL).toString();
			if (fragmentHref.isEmpty())
			{
				if (!format.isAnchor())
					return QString();

				fragmentHref = format.anchorHref();
			}

			if (href.isNull())
				href = fragmentHref;
			else if (href != fragmentHref)
				return QString();
		}
		block = block.next();
	}

	return href;
}

void TextManager::insertQuotedFragment(QTextCursor ACursor, const QTextDocumentFragment &AFragment)
{
	if (!AFragment.isEmpty())
	{
		QTextDocument doc;
		QTextCursor cursor(&doc);
		cursor.insertFragment(AFragment);

		cursor.movePosition(QTextCursor::Start);
		do { cursor.insertText("> "); } while (cursor.movePosition(QTextCursor::NextBlock));
		cursor.select(QTextCursor::Document);

		ACursor.beginEditBlock();
		if (!ACursor.atBlockStart())
			ACursor.insertText("\n");
		ACursor.insertFragment(cursor.selection());
		ACursor.insertText("\n");
		ACursor.endEditBlock();
	}
}

QTextDocumentFragment TextManager::getTrimmedTextFragment(const QTextDocumentFragment &AFragment, bool APlainText)
{
	QTextDocument doc;
	QTextCursor cursor(&doc);
	if (APlainText)
	{
		QString text = AFragment.toPlainText();
		text.remove(QChar::Null);
		text.remove(QChar::ObjectReplacementCharacter);
		cursor.insertText(text);
	}
	else
	{
		cursor.insertFragment(AFragment);
	}

	cursor.movePosition(QTextCursor::Start);
	while (cursor.movePosition(QTextCursor::NextCharacter,QTextCursor::KeepAnchor) && cursor.selectedText().trimmed().isEmpty())
		cursor.removeSelectedText();

	cursor.movePosition(QTextCursor::End);
	while (cursor.movePosition(QTextCursor::PreviousCharacter,QTextCursor::KeepAnchor) && cursor.selectedText().trimmed().isEmpty())
		cursor.removeSelectedText();

	cursor.select(QTextCursor::Document);
	return cursor.selection();
}

QString TextManager::getCopyableText(const QTextDocumentFragment &AFragment)
{
	if (AFragment.isEmpty())
		return QString();

	QTextDocument doc;
	QTextCursor cursor(&doc);
	cursor.insertFragment(AFragment);

	// The text of the links of the images is shortened for the display, the full
	// urls are copied instead of them.
	QHash<int, QPair<int, QString> > links;
	for (QTextBlock block = doc.begin(); block.isValid(); block = block.next())
	{
		for (QTextBlock::Iterator it = block.begin(); !it.atEnd(); ++it)
		{
			const QTextFragment fragment = it.fragment();
			const QTextCharFormat format = fragment.charFormat();
			if (format.hasProperty(TMI_IMAGE_LINK_URL))
				links.insert(fragment.position(), qMakePair(fragment.length(), format.stringProperty(TMI_IMAGE_LINK_URL)));
		}
	}

	// The links are replaced from the last one, so that the positions of the
	// others stay valid.
	QList<int> positions = links.keys();
	std::sort(positions.begin(), positions.end(), qGreater<int>());
	for (int position : positions)
	{
		const int length = links.value(position).first;
		const QString url = links.value(position).second;
		if (url.isEmpty() || length <= 0)
			continue;
		QTextCursor replace(&doc);
		replace.setPosition(position);
		replace.setPosition(position + length, QTextCursor::KeepAnchor);
		replace.insertText(url);
	}

	return doc.toPlainText();
}

QString TextManager::getElidedString(const QString &AString, Qt::TextElideMode AMode, int AMaxChars)
{
	if (AString.length()>AMaxChars && AMaxChars>3)
	{
		int stringChars = AMode!=Qt::ElideNone ? AMaxChars-3 : AMaxChars;

		QString string;
		if (AMode == Qt::ElideRight)
		{
			string = AString.left(stringChars);
			string.replace(QChar::LineFeed, QChar::Space);
			string.append("...");
		}
		else if (AMode == Qt::ElideLeft)
		{
			string = AString.right(stringChars);
			string.prepend("...");
		}
		else if (AMode == Qt::ElideMiddle)
		{
			QString leftString = AString.left(stringChars/2);
			QString rightString = AString.right(stringChars - stringChars/2);
			string = leftString+"..."+rightString;
		}
		else
		{
			string = AString.left(stringChars);
		}

		return string;
	}
	return AString;
}
