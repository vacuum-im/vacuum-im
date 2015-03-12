#ifndef CREATEACCOUNTWIZARD_H
#define CREATEACCOUNTWIZARD_H

#include <QMap>
#include <QLabel>
#include <QWizard>
#include <QComboBox>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QWizardPage>
#include <QRadioButton>
#include <QProgressBar>
#include <interfaces/iregistraton.h>
#include <interfaces/ixmppstreams.h>
#include <interfaces/iaccountmanager.h>
#include <interfaces/iconnectionmanager.h>


class ConnectionOptionsWidget :
	public QWidget
{
	Q_OBJECT;
	Q_PROPERTY(QString connectionEngine READ connectionEngine WRITE setConnectionEngine);
public:
	ConnectionOptionsWidget(QWidget *AParent);
	~ConnectionOptionsWidget();
	void applyOptions() const;
	void saveOptions(IAccount *AAccount) const;
public:
	QString connectionEngine() const;
	void setConnectionEngine(const QString &AEngineId);
protected slots:
	void onConnectionSettingsLinkActivated(const QString &ALink);
private:
	QLabel *lblConnectionSettings;
	IOptionsDialogWidget *odwConnectionSettings;
private:
	IConnectionEngine *FConnectionEngine;
};


class CreateAccountWizard :
	public QWizard
{
	Q_OBJECT;
public:
	enum Pages {
		PageWizardStart,
		PageAppendService,
		PageAppendSettings,
		PageAppendCheck,
		PageRegisterServer,
		PageRegisterRequest,
		PageRegisterSubmit
	};
	enum Mode {
		ModeAppend,
		ModeRegister
	};
	enum ServiceType {
		ServiceJabber,
		ServiceGoogle,
		ServiceFacebook,
		ServiceYandex,
		ServiceOdnoklassniki,
		ServiceLiveJournal,
		ServiceQIP,
		Service_Count
	};
public:
	CreateAccountWizard(QWidget *AParent = NULL);
	void accept();
};


class WizardStartPage : 
	public QWizardPage
{
	Q_OBJECT;
	Q_PROPERTY(int wizardMode READ wizardMode WRITE setWizardMode);
public:
	WizardStartPage(QWidget *AParent);
	int nextId() const;
public:
	int wizardMode() const;
	void setWizardMode(int AMode);
private:
	QRadioButton *rbtAppendAccount;
	QRadioButton *rbtRegisterAccount;
};


class AppendServicePage :
	public QWizardPage
{
	Q_OBJECT;
	Q_PROPERTY(int serviceType READ serviceType WRITE setServiceType);
public:
	AppendServicePage(QWidget *AParent);
public:
	int serviceType() const;
	void setServiceType(int AType);
protected slots:
	void onServiceButtonClicked(int AType);
private:
	int FServiceType;
	QMap<int, QRadioButton *> FTypeButton;
};


class AppendSettingsPage :
	public QWizardPage
{
	Q_OBJECT;
	Q_PROPERTY(QString accountDomain READ accountDomain WRITE setAccountDomain);
public:
	AppendSettingsPage(QWidget *AParent);
	void initializePage();
	bool isComplete() const;
	bool validatePage();
public:
	QString accountDomain() const;
	void setAccountDomain(const QString &ADomain);
	void saveAccountSettings(IAccount *AAccount) const;
private:
	QComboBox *cmbDomain;
	ConnectionOptionsWidget *cowConnOptions;
};


class AppendCheckPage :
	public QWizardPage
{
	Q_OBJECT;
public:
	AppendCheckPage(QWidget *AParent);
	~AppendCheckPage();
	void initializePage();
	void cleanupPage();
	bool isComplete() const;
	int nextId() const;
protected:
	IXmppStream *createXmppStream() const;
protected slots:
	void onXmppStreamOpened();
	void onXmppStreamError(const XmppError &AError);
private:
	QLabel *lblError;
	QLabel *lblCaption;
	QLabel *lblDescription;
	QProgressBar *prbProgress;
	QCheckBox *chbShowSettings;
private:
	bool FConnecting;
	IXmppStream *FXmppStream;
};


class RegisterServerPage :
	public QWizardPage
{
	Q_OBJECT;
	Q_PROPERTY(QString accountDomain READ accountDomain WRITE setAccountDomain);
public:
	RegisterServerPage(QWidget *AParent);
	void initializePage();
	bool validatePage();
public:
	QString accountDomain() const;
	void setAccountDomain(const QString &ADomain);
	void saveAccountSettings(IAccount *AAccount) const;
private:
	QComboBox *cmbServer;
	ConnectionOptionsWidget *cowConnOptions;
};


class RegisterRequestPage :
	public QWizardPage
{
	Q_OBJECT;
	Q_PROPERTY(QString registerId READ registerId WRITE setRegisterId);
	Q_PROPERTY(QString accountNode READ accountNode WRITE setAccountNode);
	Q_PROPERTY(QString accountPassword READ accountPassword WRITE setAccountPassword);
public:
	RegisterRequestPage(QWidget *AParent);
	~RegisterRequestPage();
	void initializePage();
	void cleanupPage();
	bool isComplete() const;
	bool validatePage();
public:
	QString registerId() const;
	void setRegisterId(const QString &AId);
	QString accountNode() const;
	void setAccountNode(const QString &ANode);
	QString accountPassword() const;
	void setAccountPassword(const QString &APassword);
protected:
	IXmppStream *createXmppStream() const;
protected slots:
	void onRegisterFields(const QString &AId, const IRegisterFields &AFields);
	void onRegisterError(const QString &AId, const XmppError &AError);
protected slots:
	void onWizardCurrentPageChanged(int APage);
private:
	QLabel *lblError;
	QLabel *lblCaption;
	QLabel *lblDescription;
	QProgressBar *prbProgress;
	QVBoxLayout *vltRegisterForm;
	IDataFormWidget *dfwRegisterForm;
private:
	bool FReinitialize;
	QString FRegisterId;
	IDataForms *FDataForms;
	IXmppStream *FXmppStream;
	IRegistration *FRegistration;
	IRegisterFields FRegisterFields;
	IRegisterSubmit FRegisterSubmit;
};


class RegisterSubmitPage :
	public QWizardPage
{
	Q_OBJECT;
public:
	RegisterSubmitPage(QWidget *AParent);
	void initializePage();
	bool isComplete() const;
	int nextId() const;
protected slots:
	void onRegisterError(const QString &AId, const XmppError &AError);
	void onRegisterSuccess(const QString &AId);
private:
	QLabel *lblError;
	QLabel *lblCaption;
	QLabel *lblDescription;
	QProgressBar *prbProgress;
	QCheckBox *chbShowSettings;
private:
	bool FRegistered;
	IRegistration *FRegistration;
};

#endif // CREATEACCOUNTWIZARD_H
