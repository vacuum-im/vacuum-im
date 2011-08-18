#include "textmanager.h"

#include <QTextBlock>

QString TextManager::getDocumentBody(const QTextDocument &ADocument)
{
	QRegExp body("<body.*>(.*)</body>");
	body.setMinimal(false);
	QString html = ADocument.toHtml();
	html = html.indexOf(body)>=0 ? body.cap(1).trimmed() : html;

	// XXX Replace <P> inserted by QTextDocument with <SPAN>
	if (html.leftRef(3).compare("<p ", Qt::CaseInsensitive) == 0 &&
		html.rightRef(4).compare("</p>", Qt::CaseInsensitive) == 0)
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
		for (QTextBlock::iterator it = block.begin(); !it.atEnd(); it++)
		{
			if (it.fragment().charFormat().isAnchor())
			{
				if (href.isNull())
					href = it.fragment().charFormat().anchorHref();
				else if (href != it.fragment().charFormat().anchorHref())
					return QString::null;
			}
			else
			{
				return QString::null;
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
		ACursor.beginEditBlock();
		if (!ACursor.atBlockStart())
			ACursor.insertText("\n");
		ACursor.insertText("> ");
		ACursor.insertFragment(AFragment);
		ACursor.insertText("\n");
		ACursor.endEditBlock();
	}
}
