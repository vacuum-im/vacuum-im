#ifndef IOPTIONSMANAGER_H
#define IOPTIONSMANAGER_H

#include <QList>
#include <QString>
#include <QDialog>
#include <QByteArray>
#include <QDomElement>
#include <utils/options.h>

#define OPTIONSMANAGER_UUID "{d29856c7-8f74-4e95-9aba-b95f4fb42f00}"

struct IOptionsDialogNode
{
	int order;
	QString nodeId;
	QString name;
	QString iconkey;
};

class IOptionsWidget
{
public:
	virtual QWidget* instance() =0;
public slots:
	virtual void apply() =0;
	virtual void reset() =0;
protected:
	virtual void modified() =0;
	virtual void childApply() =0;
	virtual void childReset() =0;
};

class IOptionsHolder
{
public:
	virtual QMultiMap<int, IOptionsWidget *> optionsWidgets(const QString &ANodeId, QWidget *AParent) =0;
};

class IOptionsManager
{
public:
	virtual QObject* instance() =0;
	//Profiles
	virtual bool isOpened() const =0;
	virtual QList<QString> profiles() const =0;
	virtual QString profilePath(const QString &AProfile) const =0;
	virtual QString lastActiveProfile() const =0;
	virtual QString currentProfile() const =0;
	virtual QByteArray currentProfileKey() const =0;
	virtual bool setCurrentProfile(const QString &AProfile, const QString &APassword) =0;
	virtual QByteArray profileKey(const QString &AProfile, const QString &APassword) const =0;
	virtual bool checkProfilePassword(const QString &AProfile, const QString &APassword) const =0;
	virtual bool changeProfilePassword(const QString &AProfile, const QString &AOldPassword, const QString &ANewPassword) =0;
	virtual bool addProfile(const QString &AProfile, const QString &APassword) =0;
	virtual bool renameProfile(const QString &AProfile, const QString &ANewName) =0;
	virtual bool removeProfile(const QString &AProfile) =0;
	virtual QDialog *showLoginDialog(QWidget *AParent = NULL) =0;
	virtual QDialog *showEditProfilesDialog(QWidget *AParent = NULL) =0;
	//OptionsDialog
	virtual QList<IOptionsHolder *> optionsHolders() const =0;
	virtual void insertOptionsHolder(IOptionsHolder *AHolder) =0;
	virtual void removeOptionsHolder(IOptionsHolder *AHolder) =0;
	virtual QList<IOptionsDialogNode> optionsDialogNodes() const =0;
	virtual IOptionsDialogNode optionsDialogNode(const QString &ANodeId) const =0;
	virtual void insertOptionsDialogNode(const IOptionsDialogNode &ANode) =0;
	virtual void removeOptionsDialogNode(const QString &ANodeId) =0;
	virtual QDialog *showOptionsDialog(const QString &ANodeId = QString::null, QWidget *AParent = NULL) =0;
	//OptionsWidgets
	virtual IOptionsWidget *optionsHeaderWidget(const QString &ACaption, QWidget *AParent) const =0;
	virtual IOptionsWidget *optionsNodeWidget(const OptionsNode &ANode, const QString &ACaption, QWidget *AParent) const =0;
protected:
	virtual void profileAdded(const QString &AProfile) =0;
	virtual void profileOpened(const QString &AProfile) =0;
	virtual void profileClosed(const QString &AProfile) =0;
	virtual void profileRenamed(const QString &AProfile, const QString &ANewName) =0;
	virtual void profileRemoved(const QString &AProfile) =0;
	virtual void optionsHolderInserted(IOptionsHolder *AHolder) =0;
	virtual void optionsHolderRemoved(IOptionsHolder *AHolder) =0;
	virtual void optionsDialogNodeInserted(const IOptionsDialogNode &ANode) =0;
	virtual void optionsDialogNodeRemoved(const IOptionsDialogNode &ANode) =0;
};

Q_DECLARE_INTERFACE(IOptionsWidget,"Vacuum.Plugin.IOptionsWidget/1.0")
Q_DECLARE_INTERFACE(IOptionsHolder,"Vacuum.Plugin.IOptionsHolder/1.0")
Q_DECLARE_INTERFACE(IOptionsManager,"Vacuum.Plugin.IOptionsManager/1.0")

#endif //IOPTIONSMANAGER_H
