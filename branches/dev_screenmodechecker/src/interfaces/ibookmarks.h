#ifndef IBOOKMARKS_H
#define IBOOKMARKS_H

#include <QUrl>
#include <QDomElement>
#include <utils/jid.h>

#define BOOKMARKS_UUID "{16C96115-9BF6-4404-BD9A-383A0EF2B346}"

struct IBookmark
{
	enum Type {
		None,
		Url,
		Conference
	};

	int type;
	QString name;
	struct {
		Jid roomJid;
		bool autojoin;
		QString nick;
		QString password;
	} conference;
	struct {
		QUrl url;
	} url;

	IBookmark() {
		type = None;
		conference.autojoin = false;
	}
	bool operator<(const IBookmark &AOther) const {
		if (type != AOther.type)
			return type < AOther.type;
		if (type == Url)
			return url.url < AOther.url.url;
		if (type == Conference)
			return conference.roomJid < AOther.conference.roomJid;
		return false;
	};
	bool operator==(const IBookmark &AOther) const {
		if (type != AOther.type)
			return false;
		if (type == Url)
			return url.url == AOther.url.url;
		if (type == Conference)
			return conference.roomJid == AOther.conference.roomJid;
		return true;
	}
};

class IBookmarks
{
public:
	virtual QObject *instance() =0;
	virtual bool isValidBookmark(const IBookmark &ABookmark) const =0;
	virtual QList<IBookmark> bookmarks(const Jid &AStreamJid) const =0;
	virtual bool addBookmark(const Jid &AStreamJid, const IBookmark &ABookmark) =0;
	virtual bool setBookmarks(const Jid &AStreamJid, const QList<IBookmark> &ABookmarks) =0;
	virtual int execEditBookmarkDialog(IBookmark *ABookmark, QWidget *AParent) const =0;
	virtual void showEditBookmarksDialog(const Jid &AStreamJid) =0;
protected:
	virtual void bookmarksChanged(const Jid &AStreamJid) =0;
};

Q_DECLARE_INTERFACE(IBookmarks,"Vacuum.Plugin.IBookmarks/1.1")

#endif // IBOOKMARKS_H
