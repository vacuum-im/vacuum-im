#ifndef IBOOKMARKS_H
#define IBOOKMARKS_H

#include <QDomElement>
#include <utils/jid.h>

#define BOOKMARKS_UUID "{16C96115-9BF6-4404-BD9A-383A0EF2B346}"

struct IBookMark
{
  QString name;
  bool autojoin;
  QString nick;
  QString password;
  QString conference;
  QString url;
};

class IBookMarks
{
public:
  virtual QObject *instance() =0;
  virtual QList<IBookMark> bookmarks(const Jid &AStreamJid) const =0;
  virtual QString addBookmark(const Jid &AStreamJid, const IBookMark &ABookmark) =0;
  virtual QString setBookmarks(const Jid &AStreamJid, const QList<IBookMark> &ABookmarks) =0;
  virtual int execEditBookmarkDialog(IBookMark *ABookmark, QWidget *AParent) const =0;
  virtual void showEditBookmarksDialog(const Jid &AStreamJid) =0;
signals:
  virtual void bookmarksUpdated(const QString &AId, const Jid &AStreamJid, const QDomElement &AElement) =0;
  virtual void bookmarksError(const QString &AId, const QString &AError) =0;
};

Q_DECLARE_INTERFACE(IBookMarks,"Vacuum.Plugin.IBookMarks/1.0")

#endif
