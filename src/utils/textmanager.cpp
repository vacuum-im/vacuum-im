#include "textmanager.h"

#include <QRegExp>
#include <QTextBlock>

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
	QString href;

	QTextDocument doc;
	doc.setHtml(AFragment.toHtml());

	QTextBlock block = doc.firstBlock();
	while (block.isValid())
	{
		for (QTextBlock::iterator it = block.begin(); !it.atEnd(); ++it)
		{
			if (it.fragment().charFormat().isAnchor())
			{
				if (href.isNull())
					href = it.fragment().charFormat().anchorHref();
				else if (href != it.fragment().charFormat().anchorHref())
					return QString();
			}
			else
			{
				return QString();
			}
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
