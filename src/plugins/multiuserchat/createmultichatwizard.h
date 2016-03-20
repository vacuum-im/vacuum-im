#ifndef CREATEMULTICHATWIZARD_H
#define CREATEMULTICHATWIZARD_H

#include <QTimer>
#include <QLabel>
#include <QWizard>
#include <QVariant>
#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>
#include <QTableView>
#include <QProgressBar>
#include <QRadioButton>
#include <QStandardItemModel>
#include <QSortFilterProxyModel>
#include <interfaces/imultiuserchat.h>
#include <interfaces/iaccountmanager.h>
#include <interfaces/iservicediscovery.h>
#include <interfaces/ivcardmanager.h>
#include <interfaces/iregistraton.h>
#include <utils/searchlineedit.h>

class CreateMultiChatWizard :
	public QWizard
{
	Q_OBJECT;
public:
	enum Mode {
		ModeJoin,
		ModeCreate,
		ModeManual,
	};
	enum Pages {
		PageMode,
		PageService,
		PageRoom,
		PageConfig,
		PageJoin,
		PageManual,
	};
public:
	CreateMultiChatWizard(QWidget *AParent = NULL);
	CreateMultiChatWizard(Mode AMode, const Jid &AStreamJid, const Jid &ARoomJid, const QString &ANick, const QString &APassword, QWidget *AParent=NULL);
	void setConfigHints(const QMap<QString,QVariant> &AHints);
public:
	void accept();
signals:
	void wizardAccepted(IMultiUserChatWindow *AWindow);
protected:
	void initialize();
};

class ModePage :
	public QWizardPage
{
	Q_OBJECT;
	Q_PROPERTY(int wizardMode READ wizardMode WRITE setWizardMode);
public:
	ModePage(QWidget *AParent);
	void initializePage();
	int nextId() const;
public:
	int wizardMode() const;
	void setWizardMode(int AMode);
private:
	QRadioButton *rbtJoinRoom;
	QRadioButton *rbtCreateRoom;
	QRadioButton *rbtManuallyRoom;
};

class ServicePage :
	public QWizardPage
{
	Q_OBJECT;
	Q_PROPERTY(QString streamJid READ streamJid WRITE setStreamJid);
	Q_PROPERTY(QString serverJid READ serverJid WRITE setServerJid);
	Q_PROPERTY(QString serviceJid READ serviceJid WRITE setServiceJid);
public:
	ServicePage(QWidget *AParent);
	void initializePage();
	bool isComplete() const;
	int nextId() const;
public:
	int wizardMode() const;
	QString streamJid() const;
	void setStreamJid(const QString &AStreamJid);
	QString serverJid() const;
	void setServerJid(const QString &AServerJid);
	QString serviceJid() const;
	void setServiceJid(const QString &AServiceJid);
protected:
	void processDiscoInfo(const IDiscoInfo &AInfo);
protected slots:
	void onCurrentAccountChanged();
	void onCurrentServerChanged();
	void onCurrentServiceChanged();
	void onAddServerButtonClicked();
	void onDiscoInfoRecieved(const IDiscoInfo &AInfo);
	void onDiscoItemsRecieved(const IDiscoItems &AItems);
private:
	QLabel *lblInfo;
	QLabel *lblAccount;
	QLabel *lblServer;
	QLabel *lblService;
	QComboBox *cmbAccount;
	QComboBox *cmbServer;
	QComboBox *cmbService;
private:
	bool FWaitItems;
	QList<Jid> FWaitInfo;
};

class RoomPage :
	public QWizardPage
{
	Q_OBJECT;
	Q_PROPERTY(QString roomJid READ roomJid WRITE setRoomJid);
protected:
	enum RoomModelColumns {
		RMC_NAME,
		RMC_USERS,
		RMC__COUNT
	};
	enum RoomModelRoles {
		RMR_ROOM_JID = Qt::UserRole+1,
		RMR_SORT
	};
public:
	RoomPage(QWidget *AParent);
	void initializePage();
	bool isComplete() const;
	int nextId() const;
public:
	int wizardMode() const;
	Jid streamJid() const;
	Jid serviceJid() const;
	QString roomJid() const;
	void setRoomJid(const QString &ARoomJid);
protected slots:
	void onRoomSearchStart();
	void onRoomNodeTextChanged();
	void onRoomNodeTimerTimeout();
	void onDiscoInfoRecieved(const IDiscoInfo &AInfo);
	void onDiscoItemsRecieved(const IDiscoItems &AItems);
	void onCurrentRoomChanged(const QModelIndex &ACurrent, const QModelIndex &APrevious);
private:
	SearchLineEdit *sleSearch;
	QTableView *tbvRoomView;
	QLabel *lblRoomNode;
	QLineEdit *lneRoomNode;
	QLabel *lblRoomDomain;
	QLabel *lblInfo;
private:
	bool FWaitInfo;
	bool FWaitItems;
	bool FRoomChecked;
	QTimer FRoomNodeTimer;
	QStandardItemModel *FRoomModel;
	QSortFilterProxyModel *FRoomProxy;
};

class ConfigPage :
	public QWizardPage
{
	Q_OBJECT;
	Q_PROPERTY(QVariant configHints READ configHints WRITE setConfigHints);
public:
	ConfigPage(QWidget *AParent);
	void cleanupPage();
	void initializePage();
	bool isComplete() const;
	bool validatePage();
	int nextId() const;
public:
	Jid streamJid() const;
	Jid roomJid() const;
	QVariant configHints() const;
	void setConfigHints(const QVariant &AHints);
protected:
	void setError(const QString &AMessage);
protected slots:
	void onConfigFormFieldChanged();
protected slots:
	void onMultiChatStateChanged(int AState);
	void onMultiChatConfigLoaded(const QString &AId, const IDataForm &AForm);
	void onMultiChatConfigUpdated(const QString &AId, const IDataForm &AForm);
	void onMultiChatRequestFailed(const QString &AId, const XmppError &AError);
private:
	QLabel *lblCaption;
	QWidget *wdtConfig;
	QProgressBar *prbProgress;
	QLabel *lblInfo;
private:
	bool FRoomCreated;
	bool FRoomConfigured;
	QString FRoomNick;
	IMultiUserChat *FMultiChat;
private:
	QString FConfigLoadRequestId;
	QString FConfigUpdateRequestId;
	IDataFormWidget *FConfigFormWidget;
	QMap<QString,QVariant> FConfigHints;
};

class JoinPage :
	public QWizardPage
{
	Q_OBJECT;
	Q_PROPERTY(QString roomNick READ roomNick WRITE setRoomNick);
	Q_PROPERTY(QString roomPassword READ roomPassword WRITE setRoomPassword);
public:
	JoinPage(QWidget *AParent);
	void initializePage();
	bool isComplete() const;
	int nextId() const;
public:
	int wizardMode() const;
	Jid streamJid() const;
	Jid roomJid() const;
	QString roomNick() const;
	void setRoomNick(const QString &ANick);
	QString roomPassword() const;
	void setRoomPassword(const QString &APassword);
protected:
	void processDiscoInfo(const IDiscoInfo &AInfo);
protected slots:
	void onRoomNickTextChanged();
	void onRoomPasswordTextChanged();
	void onRegisterNickLinkActivated();
	void onRegisterNickDialogFinished();
	void onDiscoInfoRecieved(const IDiscoInfo &AInfo);
	void onRegisteredNickRecieved(const QString &AId, const QString &ANick);
private:
	QLineEdit *lneNick;
	QLabel *lblRegister;
	QLabel *lblRoomJid;
	QLabel *lblRoomName;
	QLineEdit *lnePassword;
	QLabel *lblMucPassword;
	QLabel *lblMucMembersOnly;
	QLabel *lblMucAnonymous;
	QLabel *lblMucModerated;
	QLabel *lblMucTemporary;
	QLabel *lblMucHidden;
	QLabel *lblInfo;
private:
	bool FWaitInfo;
	bool FRoomChecked;
	IDiscoInfo FRoomInfo;
	QString FNickRequestId;
	QString FRegisteredNick;
};

class ManualPage :
	public QWizardPage
{
	Q_OBJECT;
	Q_PROPERTY(QString streamJid READ streamJid WRITE setStreamJid);
	Q_PROPERTY(QString roomJid READ roomJid WRITE setRoomJid);
	Q_PROPERTY(QString roomNick READ roomNick WRITE setRoomNick);
	Q_PROPERTY(QString roomPassword READ roomPassword WRITE setRoomPassword);
public: 
	ManualPage(QWidget *AParent);
	void initializePage();
	bool isComplete() const;
	int nextId() const;
public:
	QString streamJid() const;
	void setStreamJid(const QString &AStreamJid);
	QString roomJid() const;
	void setRoomJid(const QString &ARoomJid);
	QString roomNick() const;
	void setRoomNick(const QString &ANick);
	QString roomPassword() const;
	void setRoomPassword(const QString &APassword);
protected slots:
	void onAccountIndexChanged();
	void onRoomJidTextChanged();
	void onRoomNickTextChanged();
protected slots:
	void onRoomInfoTimerTimeout();
	void onRegisterNickLinkActivated();
	void onRegisterNickDialogFinished();
	void onDiscoInfoRecieved(const IDiscoInfo &AInfo);
	void onRegisteredNickRecieved(const QString &AId, const QString &ANick);
private:
	QComboBox *cmbAccount;
	QLineEdit *lneRoomJid;
	QLineEdit *lneRoomNick;
	QLineEdit *lneRoomPassword;
	QLabel *lblRegister;
	QLabel *lblInfo;
private:
	bool FWaitInfo;
	bool FRoomChecked;
	QTimer FRoomInfoTimer;
	QString FNickRequestId;
	QString FRegisteredNick;
};

#endif // CREATEMULTICHATWIZARD_H
