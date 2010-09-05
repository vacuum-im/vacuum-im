#ifndef OPTIONS_H
#define OPTIONS_H

#include <QObject>
#include <QVariant>
#include <QByteArray>
#include <QDomDocument>
#include "utilsexport.h"

class UTILS_EXPORT OptionsNode
{
	friend class Options;
	struct OptionsNodeData;
public:
	OptionsNode();
	OptionsNode(const OptionsNode &ANode);
	~OptionsNode();
	bool isNull() const;
	QString path() const;
	QString name() const;
	QString nspace() const;
	OptionsNode parent() const;
	QList<QString> parentNSpaces() const;
	QList<QString> childNames() const;
	QList<QString> childNSpaces(const QString &AName) const;
	bool isChildNode(const OptionsNode &ANode) const;
	QString childPath(const OptionsNode &ANode) const;
	void removeChilds(const QString &AName = QString::null, const QString &ANSpace = QString::null) const;
	OptionsNode node(const QString &APath, const QString &ANSpace = QString::null) const;
	bool hasValue(const QString &APath = QString::null, const QString &ANSpace = QString::null) const;
	QVariant value(const QString &APath = QString::null, const QString &ANSpace = QString::null) const;
	void setValue(const QVariant &AValue, const QString &APath = QString::null, const QString &ANSpace = QString::null);
public:
	bool operator==(const OptionsNode &AOther) const;
	bool operator!=(const OptionsNode &AOther) const;
	OptionsNode &operator=(const OptionsNode &AOther);
public:
	static const OptionsNode null;
private:
	OptionsNode(const QDomElement &ANode);
	OptionsNodeData *d;
};

class UTILS_EXPORT Options :
			public QObject
{
	Q_OBJECT;
	friend class OptionsNode;
	struct OptionsData;
public:
	static Options *instance();
	static bool isNull();
	static QString filesPath();
	static QByteArray cryptKey();
	static QString cleanNSpaces(const QString &APath);
	static bool hasNode(const QString &APath, const QString &ANSpace = QString::null);
	static OptionsNode node(const QString &APath, const QString &ANSpace = QString::null);
	static QVariant fileValue(const QString &APath, const QString &ANSpace = QString::null);
	static void setFileValue(const QVariant &AValue, const QString &APath, const QString &ANSpace = QString::null);
	static void setOptions(QDomDocument AOptions, const QString &AFilesPath, const QByteArray &ACryptKey);
	static QVariant defaultValue(const QString &APath);
	static void setDefaultValue(const QString &APath, const QVariant &ADefault);
	static QByteArray encrypt(const QVariant &AValue, const QByteArray &AKey = cryptKey());
	static QVariant decrypt(const QByteArray &AData, const QByteArray &AKey = cryptKey());
	static void exportNode(const QString &APath, QDomElement &AToElem);
	static void importNode(const QString &APath, const QDomElement &AFromElem);
signals:
	void optionsOpened();
	void optionsClosed();
	void optionsCreated(const OptionsNode &ANode);
	void optionsChanged(const OptionsNode &ANode);
	void optionsRemoved(const OptionsNode &ANode);
	void defaultValueChanged(const QString &APath, const QVariant &ADefault);
private:
	static OptionsData *d;
};

#endif // OPTIONS_H
