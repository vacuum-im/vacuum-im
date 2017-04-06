#include "webpage.h"

WebPage::WebPage(QWebEngineProfile *AProfile, QObject *AParent) : QWebEnginePage(AProfile, AParent)
{
}

QString WebPage::requestWebHitTest(const QPoint &APosition)
{
	static quint64 requestId = 1;

	static const QString source = QLatin1String(
		"(function() {"
		"    var e = document.elementFromPoint(%1, %2);"
		"    if (!e)"
		"        return;"
		"    function isMediaElement(e) {"
		"        return e.tagName == 'AUDIO' || e.tagName == 'VIDEO';"
		"    }"
		"    function isEditableElement(e) {"
		"        if (e.isContentEditable)"
		"            return true;"
		"        if (e.tagName == 'INPUT' || e.tagName == 'TEXTAREA')"
		"            return e.getAttribute('readonly') != 'readonly';"
		"        return false;"
		"    }"
		"    function isSelected(e) {"
		"        var selection = window.getSelection();"
		"        if (selection.type != 'Range')"
		"            return false;"
		"        return window.getSelection().containsNode(e, true);"
		"    }"
		"    var res = {"
		"        baseUrl: document.baseURI,"
		"        alternateText: e.getAttribute('alt'),"
		"        boundingRect: '',"
		"        imageUrl: '',"
		"        contentEditable: isEditableElement(e),"
		"        contentSelected: isSelected(e),"
		"        linkTitle: '',"
		"        linkUrl: '',"
		"        mediaUrl: '',"
		"        tagName: e.tagName.toLowerCase(),"
		"        innerHTML: e.innerHTML,"
		"        outerHTML: e.outerHTML"
		"    };"
		"    var r = e.getBoundingClientRect();"
		"    res.boundingRect = [r.top, r.left, r.width, r.height];"
		"    if (e.tagName == 'IMG')"
		"        res.imageUrl = e.getAttribute('src');"
		"    if (e.tagName == 'A') {"
		"        res.linkTitle = e.text;"
		"        res.linkUrl = e.getAttribute('href');"
		"    }"
		"    while (e) {"
		"        if (res.linkTitle == '' && e.tagName == 'A')"
		"            res.linkTitle = e.text;"
		"        if (res.linkUrl == '' && e.tagName == 'A')"
		"            res.linkUrl = e.getAttribute('href');"
		"        if (res.mediaUrl == '' && isMediaElement(e)) {"
		"            res.mediaUrl = e.currentSrc;"
		"            res.mediaPaused = e.paused;"
		"            res.mediaMuted = e.muted;"
		"        }"
		"        e = e.parentElement;"
		"    }"
		"    return res;"
		"})()"
	);

	QString id = QString("webhit_%1").arg(requestId++);
	QPoint viewPos = QPoint(APosition.x() / zoomFactor(), APosition.y() / zoomFactor());

	runJavaScript(source.arg(viewPos.x()).arg(viewPos.y()), [APosition, id, this](const QVariant &res) {
		WebHitTestResult result(APosition, res.toMap());
		emit webHitTestResult(id, result);
	});

	return id;
}

bool WebPage::acceptNavigationRequest(const QUrl &AUrl, QWebEnginePage::NavigationType AType, bool AIsMainFrame)
{
	if (AIsMainFrame && AType==QWebEnginePage::NavigationTypeLinkClicked)
		emit linkClicked(AUrl);
	return false;
}
