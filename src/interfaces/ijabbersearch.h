#ifndef IJABBERSEARCH_H
#define IJABBERSEARCH_H

#include <interfaces/idataforms.h>
#include <utils/jid.h>

#define JABBERSEARCH_UUID "{F66BCFC9-F6BB-43b2-8101-8CA41EF227AF}"

struct ISearchItem 
{
	Jid itemJid;
	QString firstName;
	QString lastName;
	QString nick;
	QString email;
};

struct ISearchFields 
{
	enum Fields {
		First   = 1,
		Last    = 2,
		Nick    = 4,
		Email   = 8
	};
	Jid serviceJid;
	int fieldMask;
	QString instructions;
	ISearchItem item;
	IDataForm form;
};

struct ISearchSubmit 
{
	Jid serviceJid;
	ISearchItem item;
	IDataForm form;
};

struct ISearchResult
{
	Jid serviceJid;
	QList<ISearchItem> items;
	IDataForm form;
};

class IJabberSearch 
{
public:
	virtual QObject *instance() =0;
	virtual QString sendRequest(const Jid &AStreamJid, const Jid &AServiceJid) =0;
	virtual QString sendSubmit(const Jid &AStreamJid, const ISearchSubmit &ASubmit) =0;
	virtual void showSearchDialog(const Jid &AStreamJid, const Jid &AServiceJid, QWidget *AParent = NULL) =0;
protected:
	virtual void searchFields(const QString &AId, const ISearchFields &AFields) =0;
	virtual void searchResult(const QString &AId, const ISearchResult &AResult) =0;
	virtual void searchError(const QString &AId, const QString &AError) =0;
};

Q_DECLARE_INTERFACE(IJabberSearch,"Vacuum.Plugin.IJabberSearch/1.0")

#endif
