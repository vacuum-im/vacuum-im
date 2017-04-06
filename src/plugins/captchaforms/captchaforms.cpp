#include "captchaforms.h"

#include <QTextDocument>
#include <definitions/namespaces.h>
#include <definitions/stanzahandlerorders.h>
#include <definitions/notificationtypes.h>
#include <definitions/notificationdataroles.h>
#include <definitions/notificationtypeorders.h>
#include <definitions/menuicons.h>
#include <definitions/resources.h>
#include <definitions/soundfiles.h>
#include <definitions/dataformtypes.h>
#include <utils/widgetmanager.h>
#include <utils/iconstorage.h>
#include <utils/xmpperror.h>
#include <utils/logger.h>

#define SHC_TRIGGER_IQ              "/iq"
#define SHC_TRIGGER_MESSAGE         "/message"
#define SHC_TRIGGER_PRESENCE        "/presence"
#define SHC_MESSAGE_CAPTCHA         "/message/captcha[@xmlns='" NS_CAPTCHA_FORMS "']"

#define TRIGGER_TIMEOUT             120000
#define ACCEPT_CHALLENGE_TIMEOUT    30000

static const QList<QString> CaptchaFieldTypes = QList<QString>()
	<< DATAFIELD_TYPE_TEXTSINGLE << DATAFIELD_TYPE_TEXTMULTI;

static const QList<QString> CaptchaFieldVars = QList<QString>()
	<< "qa" << "ocr"
	<< "audio_recog" << "speech_q" << "speech_recog"
	<< "picture_q" << "picture_recog"
	<< "video_q" << "video_recog";

CaptchaForms::CaptchaForms()
{
	FDataForms = NULL;
	FNotifications = NULL;
	FStanzaProcessor = NULL;
	FXmppStreamManager = NULL;
}

CaptchaForms::~CaptchaForms()
{

}

void CaptchaForms::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("CAPTCHA Forms");
	APluginInfo->description = tr("Allows to undergo tests on humanity without the use of browser");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://www.vacuum-im.org";
	APluginInfo->dependences.append(DATAFORMS_UUID);
	APluginInfo->dependences.append(XMPPSTREAMS_UUID);
	APluginInfo->dependences.append(STANZAPROCESSOR_UUID);
}

bool CaptchaForms::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);
	IPlugin *plugin = APluginManager->pluginInterface("IDataForms").value(0,NULL);
	if (plugin)
	{
		FDataForms = qobject_cast<IDataForms *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IXmppStreamManager").value(0,NULL);
	if (plugin)
	{
		FXmppStreamManager = qobject_cast<IXmppStreamManager *>(plugin->instance());
		if (FXmppStreamManager)
		{
			connect(FXmppStreamManager->instance(),SIGNAL(streamOpened(IXmppStream *)),SLOT(onXmppStreamOpened(IXmppStream *)));
			connect(FXmppStreamManager->instance(),SIGNAL(streamClosed(IXmppStream *)),SLOT(onXmppStreamClosed(IXmppStream *)));
		}
	}

	plugin = APluginManager->pluginInterface("IStanzaProcessor").value(0,NULL);
	if (plugin)
	{
		FStanzaProcessor = qobject_cast<IStanzaProcessor *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("INotifications").value(0,NULL);
	if (plugin)
	{
		FNotifications = qobject_cast<INotifications *>(plugin->instance());
		if (FNotifications)
		{
			connect(FNotifications->instance(),SIGNAL(notificationActivated(int)),SLOT(onNotificationActivated(int)));
			connect(FNotifications->instance(),SIGNAL(notificationRemoved(int)),SLOT(onNotificationRemoved(int)));
		}
	}

	return FDataForms!=NULL && FXmppStreamManager!=NULL && FStanzaProcessor!=NULL;
}

bool CaptchaForms::initObjects()
{
	if (FDataForms)
	{
		FDataForms->insertLocalizer(this,DFT_CAPTCHAFORMS);
	}
	if (FNotifications)
	{
		INotificationType notifyType;
		notifyType.order = NTO_CAPTCHA_REQUEST;
		notifyType.icon = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_CAPTCHAFORMS);
		notifyType.title = tr("When receiving a CAPTCHA challenge");
		notifyType.kindMask = INotification::PopupWindow|INotification::TrayNotify|INotification::TrayAction|INotification::SoundPlay|INotification::AlertWidget|INotification::ShowMinimized|INotification::AutoActivate;
		notifyType.kindDefs = notifyType.kindMask & ~(INotification::AutoActivate);
		FNotifications->registerNotificationType(NNT_CAPTCHA_REQUEST,notifyType);
	}
	return true;
}

bool CaptchaForms::stanzaReadWrite(int AHandleId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept)
{
	if (FDataForms && FSHIChallenge.value(AStreamJid)==AHandleId)
	{
		AAccept = true;
		IDataForm form = getChallengeForm(AStanza);
		if (!isValidChallenge(AStanza,form))
		{
			LOG_STRM_WARNING(AStreamJid,QString("Received invalid challenge from=%1, id=%2").arg(AStanza.from(),AStanza.id()));
		}
		else if (!hasTrigger(AStreamJid,form))
		{
			LOG_STRM_WARNING(AStreamJid,QString("Received unexpected challenge from=%1, id=%2").arg(AStanza.from(),AStanza.id()));
		}
		else if (!isSupportedChallenge(form))
		{
			LOG_STRM_WARNING(AStreamJid,QString("Received unsupported challenge from=%1, id=%2").arg(AStanza.from(),AStanza.id()));
			Stanza error = FStanzaProcessor->makeReplyError(AStanza,XmppStanzaError::EC_NOT_ACCEPTABLE);
			FStanzaProcessor->sendStanzaOut(AStreamJid,error);
		}
		else
		{
			QString cid = findChallenge(AStreamJid, AStanza.from());
			if (cid.isEmpty())
			{
				LOG_STRM_INFO(AStreamJid,QString("Received new challenge from=%1, id=%2").arg(AStanza.from(),AStanza.id()));
				
				ChallengeItem &item = FChallenges[AStanza.id()];
				item.streamJid = AStreamJid;
				item.challenger = AStanza.from();
				item.challengeId = AStanza.id();

				item.dialog = FDataForms->dialogWidget(FDataForms->localizeForm(form), NULL);
				item.dialog->setAllowInvalid(false);
				item.dialog->instance()->installEventFilter(this);
				IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(item.dialog->instance(),MNI_CAPTCHAFORMS,0,0,"windowIcon");
				item.dialog->instance()->setWindowTitle(tr("CAPTCHA Challenge - %1").arg(AStanza.from()));
				connect(item.dialog->instance(),SIGNAL(accepted()),SLOT(onChallengeDialogAccepted()));
				connect(item.dialog->instance(),SIGNAL(rejected()),SLOT(onChallengeDialogRejected()));

				notifyChallenge(item);
			}
			else
			{
				LOG_STRM_INFO(AStreamJid,QString("Received challenge update from=%1, id=%2").arg(AStanza.from(),cid));
				
				ChallengeItem &challenge = FChallenges[cid];
				challenge.challenger = AStanza.from();
				challenge.dialog->setForm(FDataForms->localizeForm(form));
				
				setFocusToEditableField(challenge.dialog);
			}

			emit challengeReceived(AStanza.id(), form);
			return true;
		}
	}
	else if (FSHITrigger.value(AStreamJid) == AHandleId)
	{
		appendTrigger(AStreamJid,AStanza);
	}
	return false;
}

void CaptchaForms::stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza)
{
	Q_UNUSED(AStreamJid);
	if (FChallengeRequest.contains(AStanza.id()))
	{
		QString cid = FChallengeRequest.take(AStanza.id());
		if (AStanza.isResult())
		{
			LOG_STRM_INFO(AStreamJid,QString("Challenge submit accepted by=%1, id=%2").arg(AStanza.from(),cid));
			emit challengeAccepted(cid);
		}
		else
		{
			XmppStanzaError err(AStanza);
			LOG_STRM_INFO(AStreamJid,QString("Challenge submit rejected by=%1, id=%2: %3").arg(AStanza.from(),cid,err.errorMessage()));
			emit challengeRejected(cid,err);
		}
	}
}

IDataFormLocale CaptchaForms::dataFormLocale(const QString &AFormType)
{
	IDataFormLocale locale;
	if (AFormType == DFT_CAPTCHAFORMS)
	{
		locale.title = tr("CAPTCHA Challenge");
		locale.fields["audio_recog"].label = tr("Describe the sound you hear");
		locale.fields["ocr"].label = tr("Enter the text you see");
		locale.fields["picture_q"].label = tr("Answer the question you see");
		locale.fields["picture_recog"].label = tr("Identify the picture");
		locale.fields["speech_q"].label = tr("Answer the question you hear");
		locale.fields["speech_recog"].label = tr("Enter the words you hear");
		locale.fields["video_q"].label = tr("Answer the question in the video");
		locale.fields["video_recog"].label = tr("Identify the video");
	}
	return locale;
}

bool CaptchaForms::submitChallenge(const QString &AChallengeId, const IDataForm &ASubmit)
{
	if (FDataForms && FStanzaProcessor && FChallenges.contains(AChallengeId))
	{
		ChallengeItem item = FChallenges.take(AChallengeId);
		if (FNotifications)
			FNotifications->removeNotification(FChallengeNotify.key(AChallengeId));
		item.dialog->instance()->deleteLater();

		Stanza accept(STANZA_KIND_IQ);
		accept.setType(STANZA_TYPE_SET).setTo(item.challenger.full()).setUniqueId();

		QDomElement captchaElem = accept.addElement("captcha",NS_CAPTCHA_FORMS);
		FDataForms->xmlForm(ASubmit, captchaElem);
		
		if (FStanzaProcessor->sendStanzaRequest(this,item.streamJid,accept,ACCEPT_CHALLENGE_TIMEOUT))
		{
			LOG_STRM_INFO(item.streamJid,QString("Challenge submit request sent to=%1, id=%2").arg(item.challenger.full(),AChallengeId));
			FChallengeRequest.insert(accept.id(),AChallengeId);
			emit challengeSubmited(AChallengeId, ASubmit);
			return true;
		}
		else
		{
			LOG_STRM_WARNING(item.streamJid,QString("Failed to send challenge submit request to=%1, id=%2").arg(item.challenger.full(),AChallengeId));
		}
	}
	else if (!FChallenges.contains(AChallengeId))
	{
		REPORT_ERROR("Failed to send challenge submit request: Challenge not found");
	}
	return false;
}

bool CaptchaForms::cancelChallenge(const QString &AChallengeId)
{
	if (FDataForms && FStanzaProcessor && FChallenges.contains(AChallengeId))
	{
		ChallengeItem item = FChallenges.take(AChallengeId);
		if (FNotifications)
			FNotifications->removeNotification(FChallengeNotify.key(AChallengeId));
		item.dialog->instance()->deleteLater();

		Stanza reject(STANZA_KIND_MESSAGE);
		reject.setFrom(item.challenger.full()).setId(AChallengeId);
		reject = FStanzaProcessor->makeReplyError(reject,XmppStanzaError::EC_NOT_ACCEPTABLE);

		if (FStanzaProcessor->sendStanzaOut(item.streamJid, reject))
		{
			LOG_STRM_INFO(item.streamJid,QString("Challenge cancel request sent to=%1, id=%2").arg(item.challenger.full(),AChallengeId));
			emit challengeCanceled(AChallengeId);
			return true;
		}
		else
		{
			LOG_STRM_WARNING(item.streamJid,QString("Failed to send challenge cancel request to=%1, id=%2").arg(item.challenger.full(),AChallengeId));
		}
	}
	else if (!FChallenges.contains(AChallengeId))
	{
		REPORT_ERROR("Failed to send challenge cancel request: Challenge not found");
	}
	return false;
}

void CaptchaForms::appendTrigger(const Jid &AStreamJid, const Stanza &AStanza)
{
	if (!AStanza.isResult() && !AStanza.isError())
	{
		QDateTime curDateTime = QDateTime::currentDateTime();
		
		Jid receiver = !AStanza.to().isEmpty() ? AStanza.to() : AStreamJid.domain();
		QList<TriggerItem> &triggers = FTriggers[AStreamJid][receiver];

		TriggerItem item;
		item.id = AStanza.id();
		item.sent = curDateTime;

		for(QList<TriggerItem>::iterator it=triggers.begin(); it!=triggers.end(); )
		{
			if (it->sent.msecsTo(curDateTime) > TRIGGER_TIMEOUT)
				it = triggers.erase(it);
			else if (it->id == item.id)
				it = triggers.erase(it);
			else
				++it;
		}

		triggers.prepend(item);
	}
}

bool CaptchaForms::hasTrigger(const Jid &AStreamJid, const IDataForm &AForm) const
{
	if (FDataForms)
	{
		QString sid = FDataForms->fieldValue("sid",AForm.fields).toString();
		Jid from = FDataForms->fieldValue("from",AForm.fields).toString();

		QDateTime curDateTime = QDateTime::currentDateTime();
		QList<TriggerItem> triggers = FTriggers.value(AStreamJid).value(from);
		foreach(const TriggerItem &item, triggers)
		{
			if (item.id==sid && item.sent.msecsTo(curDateTime)<TRIGGER_TIMEOUT)
				return true;
		}
	}
	return false;
}

IDataForm CaptchaForms::getChallengeForm(const Stanza &AStanza) const
{
	QDomElement formElem = AStanza.firstElement("captcha",NS_CAPTCHA_FORMS).firstChildElement("x");
	while (!formElem.isNull() && formElem.namespaceURI()!=NS_JABBER_DATA)
		formElem = formElem.nextSiblingElement("x");
	return FDataForms!=NULL ? FDataForms->dataForm(formElem) : IDataForm();
}

bool CaptchaForms::isValidChallenge(const Stanza &AStanza, const IDataForm &AForm) const
{
	if (FDataForms)
	{
		if (AStanza.id().isEmpty())
			return false;

		if (FDataForms->fieldValue("FORM_TYPE",AForm.fields).toString() != NS_CAPTCHA_FORMS)
			return false;

		Jid fromAttr = AStanza.from();
		Jid fromField = FDataForms->fieldValue("from",AForm.fields).toString();
		if (fromAttr.pBare()!=fromField.pBare() && fromAttr.pBare()!=fromField.pDomain())
			return false;

		/* This checks removed due to capability issues with entities that does not follow XEP
		if (AStanza.id()!=FDataForms->fieldValue("challenge",AForm.fields).toString())
			return false;
		*/

		return true;
	}
	return false;
}

bool CaptchaForms::isSupportedChallenge(IDataForm &AForm) const
{
	if (FDataForms)
	{
		int availAnswers = 0;
		for (int i=0; i<AForm.fields.count(); i++)
		{
			IDataField &field = AForm.fields[i];
			if (CaptchaFieldVars.contains(field.var))
			{
				if (field.media.uris.isEmpty() || FDataForms->isSupportedMedia(field.media))
					availAnswers++;
				else if (!field.required)
					field.type = DATAFIELD_TYPE_HIDDEN;
				else
					return false;
			}
		}

		int minAnswers = FDataForms->fieldIndex("answers",AForm.fields)>=0 ? FDataForms->fieldValue("answers",AForm.fields).toInt() : 1;
		return availAnswers >= minAnswers;
	}
	return false;
}

void CaptchaForms::notifyChallenge(const ChallengeItem &AChallenge)
{
	if (FNotifications)
	{
		INotification notify;
		notify.kinds = FNotifications->enabledTypeNotificationKinds(NNT_CAPTCHA_REQUEST);
		if (notify.kinds > 0)
		{
			notify.typeId = NNT_CAPTCHA_REQUEST;
			notify.data.insert(NDR_ICON,IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_CAPTCHAFORMS));
			notify.data.insert(NDR_POPUP_TITLE,FNotifications->contactName(AChallenge.streamJid,AChallenge.challenger));
			notify.data.insert(NDR_POPUP_IMAGE,FNotifications->contactAvatar(AChallenge.challenger));
			notify.data.insert(NDR_POPUP_CAPTION, tr("CAPTCHA Challenge"));
			notify.data.insert(NDR_POPUP_TEXT,tr("You have received the CAPTCHA challenge"));
			notify.data.insert(NDR_SOUND_FILE,SDF_CAPTCHAFORMS_REQUEST);
			notify.data.insert(NDR_ALERT_WIDGET,(qint64)AChallenge.dialog->instance());
			notify.data.insert(NDR_SHOWMINIMIZED_WIDGET,(qint64)AChallenge.dialog->instance());
			FChallengeNotify.insert(FNotifications->appendNotification(notify),AChallenge.challengeId);
		}
		else
		{
			AChallenge.dialog->instance()->show();
		}
	}
}

QString CaptchaForms::findChallenge(IDataDialogWidget *ADialog) const
{
	for (QMap<QString, ChallengeItem>::const_iterator it=FChallenges.constBegin(); it!=FChallenges.constEnd(); ++it)
	{
		if (it->dialog == ADialog)
			return it.key();
	}
	return QString::null;
}

QString CaptchaForms::findChallenge(const Jid &AStreamJid, const Jid &AContactJid) const
{
	for (QMap<QString, ChallengeItem>::const_iterator it=FChallenges.constBegin(); it!=FChallenges.constEnd(); ++it)
	{
		if (AStreamJid==it->streamJid && AContactJid==it->challenger)
			return it.key();
	}
	return QString::null;
}

bool CaptchaForms::setFocusToEditableField(IDataDialogWidget *ADialog)
{
	if (FDataForms)
	{
		IDataFieldWidget *focusField = NULL;
		foreach(const IDataField &field, ADialog->formWidget()->dataForm().fields)
		{
			if (CaptchaFieldTypes.contains(field.type) && CaptchaFieldVars.contains(field.var))
			{
				if (!FDataForms->isMediaValid(field.media) || FDataForms->isSupportedMedia(field.media))
				{
					if (field.required)
					{
						focusField = ADialog->formWidget()->fieldWidget(field.var);
						break;
					}
					else if (focusField == NULL)
					{
						focusField = ADialog->formWidget()->fieldWidget(field.var);
					}
				}
			}
		}

		if (focusField != NULL)
		{
			focusField->instance()->setFocus();
			return true;
		}
		else
		{
			LOG_WARNING("Failed to set focus to editable field");
		}
	}
	return false;
}

bool CaptchaForms::eventFilter(QObject *AObject, QEvent *AEvent)
{
	if (AEvent->type() == QEvent::WindowActivate)
	{
		IDataDialogWidget *dialog = qobject_cast<IDataDialogWidget *>(AObject);
		if (dialog)
		{
			if (FNotifications)
			{
				QString cid = findChallenge(dialog);
				FNotifications->removeNotification(FChallengeNotify.key(cid));
			}
			setFocusToEditableField(dialog);
		}
	}
	return QObject::eventFilter(AObject,AEvent);
}

void CaptchaForms::onXmppStreamOpened(IXmppStream *AXmppStream)
{
	if (FStanzaProcessor)
	{
		IStanzaHandle triggerHandle;
		triggerHandle.handler = this;
		triggerHandle.order = SHO_IMPO_CAPTCHAFORMS;
		triggerHandle.direction = IStanzaHandle::DirectionOut;
		triggerHandle.streamJid = AXmppStream->streamJid();
		triggerHandle.conditions.append(SHC_TRIGGER_IQ);
		triggerHandle.conditions.append(SHC_TRIGGER_MESSAGE);
		triggerHandle.conditions.append(SHC_TRIGGER_PRESENCE);
		FSHITrigger.insert(triggerHandle.streamJid, FStanzaProcessor->insertStanzaHandle(triggerHandle));

		IStanzaHandle challengeHandle;
		challengeHandle.handler = this;
		challengeHandle.order = SHO_MI_CAPTCHAFORMS;
		challengeHandle.direction = IStanzaHandle::DirectionIn;
		challengeHandle.streamJid = AXmppStream->streamJid();
		challengeHandle.conditions.append(SHC_MESSAGE_CAPTCHA);
		FSHIChallenge.insert(challengeHandle.streamJid, FStanzaProcessor->insertStanzaHandle(challengeHandle));
	}
}

void CaptchaForms::onXmppStreamClosed(IXmppStream *AXmppStream)
{
	QList<IDataDialogWidget *> dialogs;
	for (QMap<QString, ChallengeItem>::const_iterator it=FChallenges.constBegin(); it!=FChallenges.constEnd(); ++it)
		if (it->streamJid == AXmppStream->streamJid())
			dialogs.append(it->dialog);

	foreach(IDataDialogWidget *dialog, dialogs)
		dialog->instance()->reject();

	if (FStanzaProcessor)
	{
		FStanzaProcessor->removeStanzaHandle(FSHITrigger.take(AXmppStream->streamJid()));
		FStanzaProcessor->removeStanzaHandle(FSHIChallenge.take(AXmppStream->streamJid()));
	}

	FTriggers.remove(AXmppStream->streamJid());
}

void CaptchaForms::onChallengeDialogAccepted()
{
	QString cid = findChallenge(qobject_cast<IDataDialogWidget *>(sender()));
	if (!cid.isEmpty())
	{
		const ChallengeItem &item = FChallenges.value(cid);
		submitChallenge(cid,item.dialog->formWidget()->submitDataForm());
	}
	else
	{
		REPORT_ERROR("Failed to accept challenge by dialog: Challenge not found");
	}
}

void CaptchaForms::onChallengeDialogRejected()
{
	QString cid = findChallenge(qobject_cast<IDataDialogWidget *>(sender()));
	if (!cid.isEmpty())
		cancelChallenge(cid);
	else
		REPORT_ERROR("Failed to cancel challenge by dialog: Challenge not found");
}

void CaptchaForms::onNotificationActivated(int ANotifyId)
{
	QString cid = FChallengeNotify.value(ANotifyId);
	if (FChallenges.contains(cid))
	{
		WidgetManager::showActivateRaiseWindow(FChallenges.value(cid).dialog->instance());
		FNotifications->removeNotification(ANotifyId);
	}
}

void CaptchaForms::onNotificationRemoved(int ANotifyId)
{
	QString cid = FChallengeNotify.value(ANotifyId);
	if (FChallenges.contains(cid))
	{
		IDataDialogWidget *dialog = FChallenges.value(cid).dialog;
		if (!dialog->instance()->isVisible())
			dialog->instance()->reject();
	}
	FChallengeNotify.remove(ANotifyId);
}
