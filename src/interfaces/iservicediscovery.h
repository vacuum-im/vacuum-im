#ifndef ISERVICEDISCOVERY_H
#define ISERVICEDISCOVERY_H

#include <QIcon>
#include <QStringList>
#include <QTreeWidgetItem>
#include <interfaces/ipluginmanager.h>
#include <interfaces/idataforms.h>
#include <utils/jid.h>
#include <utils/xmpperror.h>
#include <utils/toolbarchanger.h>

#define SERVICEDISCOVERY_UUID "{CF0D99D1-A2D8-4583-87FD-E584E0915BCC}"

struct IDiscoIdentity
{
	QString category;
	QString type;
	QString lang;
	QString name;
};

struct IDiscoItem
{
	Jid itemJid;
	QString node;
	QString name;
};

struct IDiscoFeature
{
	IDiscoFeature() { 
		active = false;
	}
	bool active;
	QIcon icon;
	QString var;
	QString name;
	QString description;
};

struct IDiscoInfo
{
	Jid streamJid;
	Jid contactJid;
	QString node;
	QList<IDiscoIdentity> identity;
	QStringList features;
	QList<IDataForm> extensions;
	XmppStanzaError error;
};

struct IDiscoItems
{
	Jid streamJid;
	Jid contactJid;
	QString node;
	QList<IDiscoItem> items;
	XmppStanzaError error;
};

class IDiscoHandler
{
public:
	virtual void fillDiscoInfo(IDiscoInfo &ADiscoInfo) =0;
	virtual void fillDiscoItems(IDiscoItems &ADiscoItems) =0;
};

class IDiscoFeatureHandler
{
public:
	virtual bool execDiscoFeature(const Jid &AStreamJid, const QString &AFeature, const IDiscoInfo &ADiscoInfo) =0;
	virtual Action *createDiscoFeatureAction(const Jid &AStreamJid, const QString &AFeature, const IDiscoInfo &ADiscoInfo, QWidget *AParent) =0;
};

class IDiscoItemsWindow
{
public:
	virtual QWidget *instance() =0;
	virtual Jid streamJid() const =0;
	virtual ToolBarChanger *toolBarChanger() const =0;
	virtual ToolBarChanger *actionsBarChanger() const =0;
	virtual void discover(const Jid AContactJid, const QString &ANode) =0;
protected:
	virtual void discoverChanged(const Jid AContactJid, const QString &ANode) =0;
	virtual void currentIndexChanged(const QModelIndex &AIndex) =0;
	virtual void indexContextMenu(const QModelIndex &AIndex, Menu *AMenu) =0;
	virtual void windowDestroyed(IDiscoItemsWindow *AWindow) =0;
};

class IServiceDiscovery
{
public:
	virtual QObject *instance() =0;
	virtual IPluginManager *pluginManager() const =0;
	virtual IDiscoInfo selfDiscoInfo(const Jid &AStreamJid, const QString &ANode = QString::null) const =0;
	virtual void showDiscoInfo(const Jid &AStreamJid, const Jid &AContactJid, const QString &ANode, QWidget *AParent = NULL) =0;
	virtual void showDiscoItems(const Jid &AStreamJid, const Jid &AContactJid, const QString &ANode, QWidget *AParent = NULL) =0;
	virtual bool checkDiscoFeature(const Jid &AStreamJid, const Jid &AContactJid, const QString &ANode, const QString &AFeature, bool ADefault = true) =0;
	virtual QList<IDiscoInfo> findDiscoInfo(const Jid &AStreamJid, const IDiscoIdentity &AIdentity, const QStringList &AFeatures, const IDiscoItem &AParent) const =0;
	virtual QIcon identityIcon(const QList<IDiscoIdentity> &AIdentity) const =0;
	virtual QIcon serviceIcon(const Jid &AStreamJid, const Jid AItemJid, const QString &ANode) const =0;
	virtual void updateSelfEntityCapabilities() =0;
	//DiscoHandler
	virtual void insertDiscoHandler(IDiscoHandler *AHandler) =0;
	virtual void removeDiscoHandler(IDiscoHandler *AHandler) =0;
	//FeatureHandler
	virtual bool hasFeatureHandler(const QString &AFeature) const =0;
	virtual void insertFeatureHandler(const QString &AFeature, IDiscoFeatureHandler *AHandler, int AOrder) =0;
	virtual bool execFeatureHandler(const Jid &AStreamJid, const QString &AFeature, const IDiscoInfo &ADiscoInfo) =0;
	virtual QList<Action *> createFeatureActions(const Jid &AStreamJid, const QString &AFeature, const IDiscoInfo &ADiscoInfo, QWidget *AParent) =0;
	virtual void removeFeatureHandler(const QString &AFeature, IDiscoFeatureHandler *AHandler) =0;
	//DiscoFeatures
	virtual void insertDiscoFeature(const IDiscoFeature &AFeature) =0;
	virtual QList<QString> discoFeatures() const =0;
	virtual IDiscoFeature discoFeature(const QString &AFeatureVar) const =0;
	virtual void removeDiscoFeature(const QString &AFeatureVar) =0;
	//DiscoInfo
	virtual bool hasDiscoInfo(const Jid &AStreamJid, const Jid &AContactJid, const QString &ANode = QString::null) const =0;
	virtual IDiscoInfo discoInfo(const Jid &AStreamJid, const Jid &AContactJid, const QString &ANode = QString::null) const =0;
	virtual bool requestDiscoInfo(const Jid &AStreamJid, const Jid &AContactJid, const QString &ANode = QString::null) =0;
	virtual void removeDiscoInfo(const Jid &AStreamJid, const Jid &AContactJid, const QString &ANode = QString::null) =0;
	virtual int findIdentity(const QList<IDiscoIdentity> &AIdentity, const QString &ACategory, const QString &AType) const =0;
	//DiscoItems
	virtual bool requestDiscoItems(const Jid &AStreamJid, const Jid &AContactJid, const QString &ANode = QString::null) =0;
protected:
	virtual void discoItemsWindowCreated(IDiscoItemsWindow *AWindow) =0;
	virtual void discoItemsWindowDestroyed(IDiscoItemsWindow *AWindow) =0;
	virtual void discoHandlerInserted(IDiscoHandler *AHandler) =0;
	virtual void discoHandlerRemoved(IDiscoHandler *AHandler) =0;
	virtual void featureHandlerInserted(const QString &AFeature, IDiscoFeatureHandler *AHandler) =0;
	virtual void featureHandlerRemoved(const QString &AFeature, IDiscoFeatureHandler *AHandler) =0;
	virtual void discoFeatureInserted(const IDiscoFeature &AFeature) =0;
	virtual void discoFeatureRemoved(const IDiscoFeature &AFeature) =0;
	virtual void discoInfoReceived(const IDiscoInfo &ADiscoInfo) =0;
	virtual void discoInfoRemoved(const IDiscoInfo &ADiscoInfo) =0;
	virtual void discoItemsReceived(const IDiscoItems &ADiscoItems) =0;
};

Q_DECLARE_INTERFACE(IDiscoHandler,"Vacuum.Plugin.IDiscoHandler/1.0")
Q_DECLARE_INTERFACE(IDiscoFeatureHandler,"Vacuum.Plugin.IDiscoFeatureHandler/1.0")
Q_DECLARE_INTERFACE(IDiscoItemsWindow,"Vacuum.Plugin.IDiscoItemsWindow/1.0")
Q_DECLARE_INTERFACE(IServiceDiscovery,"Vacuum.Plugin.IServiceDiscovery/1.0")

#endif
