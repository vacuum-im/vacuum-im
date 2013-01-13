#ifndef IEMOTICONS_H
#define IEMOTICONS_H

#include <QUrl>
#include <QIcon>
#include <QString>
#include <QStringList>

#define EMOTICONS_UUID "{B22901A6-4CDC-4218-A0C9-831131DDC8BA}"

class IEmoticons
{
public:
	virtual QObject *instance() =0;
	virtual QList<QString> activeIconsets() const =0;
	virtual QUrl urlByKey(const QString &AKey) const =0;
	virtual QString keyByUrl(const QUrl &AUrl) const =0;
};

Q_DECLARE_INTERFACE(IEmoticons,"Vacuum.Plugin.IEmoticons/1.0")

#endif
