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

#define SHC_MESSAGE_CAPTCHA         "/message/captcha[@xmlns='" NS_CAPTCHA_FORMS "']"

#define ACCEPT_CHALLENGE_TIMEOUT    30000

CaptchaForms::CaptchaForms()
{
	FDataForms = NULL;
	FXmppStreams = NULL;
	FNotifications = NULL;
	FStanzaProcessor = NULL;
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

	plugin = APluginManager->pluginInterface("IXmppStreams").value(0,NULL);
	if (plugin)
	{
		FXmppStreams = qobject_cast<IXmppStreams *>(plugin->instance());
		if (FXmppStreams)
		{
			connect(FXmppStreams->instance(),SIGNAL(opened(IXmppStream *)),SLOT(onStreamOpened(IXmppStream *)));
			connect(FXmppStreams->instance(),SIGNAL(closed(IXmppStream *)),SLOT(onStreamClosed(IXmppStream *)));
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

	return FDataForms!=NULL && FXmppStreams!=NULL && FStanzaProcessor!=NULL;
}

bool CaptchaForms::initObjects()
{
	if (FDataForms)
	{
		FDataForms->insertLocalizer(this,DATA_FORM_CAPTCHAFORMS);
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
		IDataForm form;
		if (!isValidChallenge(AStreamJid,AStanza,form))
		{
			LOG_STRM_WARNING(AStreamJid,QString("Received invalid challenge from=%1, id=%2").arg(AStanza.from(),AStanza.id()));
		}
		else if (!isSupportedChallenge(form))
		{
			LOG_STRM_WARNING(AStreamJid,QString("Received unsupported challenge from=%1, id=%2").arg(AStanza.from(),AStanza.id()));
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
				setFocusToEditableWidget(challenge.dialog->instance());
			}
			emit challengeReceived(AStanza.id(), form);
			return true;
		}
	}
	else if (FDataForms)
	{
		REPORT_ERROR("Received unexpected stanza");
	}
	return false;
}

void CaptchaForms::stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza)
{
	Q_UNUSED(AStreamJid);
	if (FChallengeRequest.contains(AStanza.id()))
	{
		QString cid = FChallengeRequest.take(AStanza.id());
		if (AStanza.type() == "result")
		{
			LOG_STRM_INFO(AStreamJid,QString("Challenge submit accepted by=%1, id=%2").arg(AStanza.from(),cid));
			emit challengeAccepted(cid);
		}
		else
		{
			LOG_STRM_INFO(AStreamJid,QString("Challenge submit rejected by=%1, id=%2").arg(AStanza.from(),cid));
			emit challengeRejected(cid, XmppStanzaError(AStanza));
		}
	}
}

IDataFormLocale CaptchaForms::dataFormLocale(const QString &AFormType)
{
	IDataFormLocale locale;
	if (AFormType == DATA_FORM_CAPTCHAFORMS)
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

		Stanza accept("iq");
		accept.setType("set");
		accept.setId(FStanzaProcessor->newId());
		accept.setTo(item.challenger.full());
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

		Stanza reject("message");
		reject.setId(AChallengeId).setFrom(item.challenger.full());
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

bool CaptchaForms::isSupportedChallenge(IDataForm &AForm) const
{
	static QStringList methods = QStringList() << "qa" << "ocr" << "picture_q" << "picture_recog";
	if (FDataForms)
	{
		int answers = 0;
		for (int i=0; i<AForm.fields.count(); i++)
		{
			IDataField &field = AForm.fields[i];
			if (methods.contains(field.var))
			{
				bool accepted = field.media.uris.isEmpty();
				for (int i=0; !accepted && i<field.media.uris.count(); i++)
					accepted = FDataForms->isSupportedUri(field.media.uris.at(i));
				if (accepted)
					answers++;
				else
					field.type = DATAFIELD_TYPE_HIDDEN;
			}
			else if (!field.required || !field.value.isNull())
			{
				field.type = DATAFIELD_TYPE_HIDDEN;
			}
			else
			{
				return false;
			}
		}
		int minAnswers = FDataForms->fieldIndex("answers",AForm.fields)>=0 ? FDataForms->fieldValue("answers",AForm.fields).toInt() : 1;
		return answers >= minAnswers;
	}
	return false;
}

bool CaptchaForms::isValidChallenge(const Jid &AStreamJid, const Stanza &AStanza, IDataForm &AForm) const
{
	Q_UNUSED(AStreamJid);
	if (FDataForms)
	{
		QDomElement formElem = AStanza.firstElement("captcha",NS_CAPTCHA_FORMS).firstChildElement("x");
		while (!formElem.isNull() && formElem.namespaceURI()!=NS_JABBER_DATA)
			formElem = formElem.nextSiblingElement("x");
		AForm = FDataForms->dataForm(formElem);

		if (AStanza.id().isEmpty())
			return false;
		if (FDataForms->fieldValue("FORM_TYPE",AForm.fields).toString() != NS_CAPTCHA_FORMS)
			return false;
		
		/* This checks removed due to capability issues with entities that does not follow XEP
		Jid fromAttr = AStanza.from();
		Jid fromField = FDataForms->fieldValue("from",AForm.fields).toString();
		if (fromAttr.pBare()!=fromField.pBare() && fromAttr.pBare()!=fromField.pDomain())
			return false;
		if (AStanza.id()!=FDataForms->fieldValue("challenge",AForm.fields).toString())
			return false;
		*/

		return true;
	}
	return false;
}

void CaptchaForms::notifyChallenge(const ChallengeItem &AChallenge)
{
	if (FDataForms && FNotifications)
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
			return;
		}
	}
	AChallenge.dialog->instance()->show();
}

QString CaptchaForms::findChallenge(IDataDialogWidget *ADialog) const
{
	QMap<QString, ChallengeItem>::const_iterator it = FChallenges.constBegin();
	while (it != FChallenges.constEnd())
	{
		if (it->dialog == ADialog)
			return it.key();
		++it;
	}
	return QString::null;
}

QString CaptchaForms::findChallenge(const Jid &AStreamJid, const Jid &AContactJid) const
{
	if (FDataForms && AContactJid.isValid())
	{
		QMap<QString, ChallengeItem>::const_iterator it = FChallenges.constBegin();
		while (it != FChallenges.constEnd())
		{
			if (AStreamJid==it->streamJid && AContactJid==it->challenger)
				return it.key();
			++it;
		}
	}
	else if (!AContactJid.isValid())
	{
		REPORT_ERROR("Failed to find challenge: Invalid contact");
	}
	return QString::null;
}

bool CaptchaForms::setFocusToEditableWidget(QWidget *AWidget)
{
	static const QList<const char *> editableWidgets = QList<const char *>() << "QLineEdit" << "QTextEdit";

	QWidget *focus = AWidget->focusWidget();
	foreach(const char *className, editableWidgets)
	{
		if (focus && focus->inherits(className))
		{
			return true;
		}
		else if (AWidget->focusPolicy()!=Qt::NoFocus && AWidget->inherits(className))
		{
			AWidget->setFocus();
			return true;
		}
	}

	foreach(QObject *child, AWidget->children())
		if (child->isWidgetType() && setFocusToEditableWidget(qobject_cast<QWidget *>(child)))
			return true;

	LOG_WARNING("Failed to set focus to editable widget");

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
			setFocusToEditableWidget(dialog->instance());
		}
	}
	return QObject::eventFilter(AObject,AEvent);
}

void CaptchaForms::onStreamOpened(IXmppStream *AXmppStream)
{
	if (FStanzaProcessor)
	{
		IStanzaHandle handle;
		handle.handler = this;
		handle.order = SHO_MI_CAPTCHAFORMS;
		handle.direction = IStanzaHandle::DirectionIn;
		handle.streamJid = AXmppStream->streamJid();
		handle.conditions.append(SHC_MESSAGE_CAPTCHA);
		FSHIChallenge.insert(handle.streamJid, FStanzaProcessor->insertStanzaHandle(handle));
	}
}

void CaptchaForms::onStreamClosed(IXmppStream *AXmppStream)
{
	QList<IDataDialogWidget *> dialogs;

	QMap<QString, ChallengeItem>::const_iterator it = FChallenges.constBegin();
	while (it != FChallenges.constEnd())
	{
		if (it->streamJid == AXmppStream->streamJid())
			dialogs.append(it->dialog);
		++it;
	}

	foreach(IDataDialogWidget *dialog, dialogs)
		dialog->instance()->reject();

	if (FStanzaProcessor)
		FStanzaProcessor->removeStanzaHandle(FSHIChallenge.take(AXmppStream->streamJid()));
}

void CaptchaForms::onChallengeDialogAccepted()
{
	QString cid = findChallenge(qobject_cast<IDataDialogWidget *>(sender()));
	if (!cid.isEmpty())
	{
		const ChallengeItem &item = FChallenges.value(cid);
		submitChallenge(cid,FDataForms->dataSubmit(item.dialog->formWidget()->userDataForm()));
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

Q_EXPORT_PLUGIN2(plg_captchaforms, CaptchaForms)
