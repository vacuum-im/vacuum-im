#ifndef IBOOKMARKS_H
#define IBOOKMARKS_H

#include <QUrl>
#include <QDialog>
#include <QDomElement>
#include <utils/jid.h>

#define BOOKMARKS_UUID "{16C96115-9BF6-4404-BD9A-383A0EF2B346}"

struct IBookmark
{
	enum Type {
		TypeNone,
		TypeUrl,
		TypeRoom
	};

	int type;
	QString name;
	struct {
		QUrl url;
	} url;
	struct {
		Jid roomJid;
		QString nick;
		QString password;
		bool autojoin;
	} room;

	IBookmark() {
		type = TypeNone;
		room.autojoin = false;
	}
	inline bool isNull() const {
		return type==TypeNone;
	}
	inline bool isValid() const {
		if (type == TypeUrl)
			return url.url.isValid();
		if (type == TypeRoom)
			return room.roomJid.isValid();
		return false;
	}
	inline bool operator<(const IBookmark &AOther) const {
		if (type != AOther.type)
			return type < AOther.type;
		if (type == TypeUrl)
			return url.url < AOther.url.url;
		if (type == TypeRoom)
			return room.roomJid < AOther.room.roomJid;
		return false;
	};
	inline bool operator==(const IBookmark &AOther) const {
		if (type != AOther.type)
			return false;
		if (type == TypeUrl)
			return url.url == AOther.url.url;
		if (type == TypeRoom)
			return room.roomJid == AOther.room.roomJid;
		return true;
	}
};

class IBookmarks
{
public:
	virtual QObject *instance() =0;
	virtual bool isReady(const Jid &AStreamJid) const =0;
	virtual QList<IBookmark> bookmarks(const Jid &AStreamJid) const =0;
	virtual bool addBookmark(const Jid &AStreamJid, const IBookmark &ABookmark) =0;
	virtual bool setBookmarks(const Jid &AStreamJid, const QList<IBookmark> &ABookmarks) =0;
	virtual QDialog *showEditBookmarkDialog(IBookmark *ABookmark, QWidget *AParent = NULL) =0;
	virtual QDialog *showEditBookmarksDialog(const Jid &AStreamJid, QWidget *AParent = NULL) =0;
protected:
	virtual void bookmarksOpened(const Jid &AStreamJid) =0;
	virtual void bookmarksClosed(const Jid &AStreamJid) =0;
	virtual void bookmarksChanged(const Jid &AStreamJid) =0;
};

Q_DECLARE_INTERFACE(IBookmarks,"Vacuum.Plugin.IBookmarks/1.3")

#endif // IBOOKMARKS_H
