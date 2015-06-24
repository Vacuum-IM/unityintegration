#include "unityintegration.h"
#include <utils/logger.h>

UnityIntegration::UnityIntegration()
{
	FLauncherCount = 0;
	FFileStreamsManager = NULL;
	FMainWindowPlugin = NULL;
	FMultiUserChatManager = NULL;
	FNotifications = NULL;
	FOptionsManager = NULL;
	FPluginManager = NULL;
	FStatusChanger = NULL;
	FAvatars = NULL;
}

template<typename T> void UnityIntegration::sendMessage(const char *name, const T& val)
{
	QVariantMap map;
	map.insert(QLatin1String(name), val);
	sendMessage(map);
}

void UnityIntegration::sendMessage(const QVariantMap &map)
{
	QDBusMessage message = QDBusMessage::createSignal("/vacuum", "com.canonical.Unity.LauncherEntry", "Update");
	QVariantList args;
	args << QLatin1String("application://vacuum.desktop") << map;
	message.setArguments(args);
	if (!QDBusConnection::sessionBus().send(message))
		LOG_WARNING(QString("Unable to send message"));
}

UnityIntegration::~UnityIntegration()
{
}

void UnityIntegration::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Unity Integration");
	APluginInfo->description = tr("Provides integration with Unity");
	APluginInfo->version = "1.3";
	APluginInfo->author = "Alexey Ivanov";
	APluginInfo->homePage = "https://github.com/Vacuum-IM/unityintegration";
	APluginInfo->dependences.append(AVATARTS_UUID);
	APluginInfo->dependences.append(NOTIFICATIONS_UUID);
	APluginInfo->dependences.append(STATUSCHANGER_UUID);
}

bool UnityIntegration::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	AInitOrder=100;

	FUnityInterface = new QDBusInterface("com.canonical.Unity","/com/canonical/Unity/Launcher","org.freedesktop.DBus.Properties",QDBusConnection::sessionBus());

	if(FUnityInterface->lastError().type() != QDBusError::NoError)
	{
		LOG_WARNING(QString("Plugin cannot work properly, your DE probably not Ubuntu Unity."));
		return false;
	}

	FPluginManager = APluginManager;
	connect(FPluginManager->instance(),SIGNAL(shutdownStarted()),this,SLOT(onShutdownStarted()));

	IPlugin *plugin = APluginManager->pluginInterface("IMultiUserChatPlugin").value(0,NULL);
	if (plugin)
		FMultiUserChatManager = qobject_cast<IMultiUserChatManager *>(plugin->instance());

	plugin = APluginManager->pluginInterface("INotifications").value(0,NULL);
	if (plugin)
	{
		FNotifications = qobject_cast<INotifications *>(plugin->instance());
		if (FNotifications)
		{
			connect(FNotifications->instance(),SIGNAL(notificationAppended(int, const INotification)),this,SLOT(onNotificationAdded(int, const INotification)));
			connect(FNotifications->instance(),SIGNAL(notificationRemoved(int)),this,SLOT(onNotificationRemoved(int)));
		}
	}

	plugin = APluginManager->pluginInterface("IMainWindowPlugin").value(0,NULL);
	if (plugin)
		FMainWindowPlugin = qobject_cast<IMainWindowPlugin *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IOptionsManager").value(0,NULL);
	if (plugin)
		FOptionsManager = qobject_cast<IOptionsManager *>(plugin->instance());
		if (FOptionsManager)
			connect(FOptionsManager->instance(),SIGNAL(profileOpened(const QString)),this,SLOT(onProfileOpened(const QString)));

	plugin = APluginManager->pluginInterface("IFileStreamsManager").value(0,NULL);
	if (plugin)
		FFileStreamsManager = qobject_cast<IFileStreamsManager *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IAvatars").value(0,NULL);
	if (plugin)
		FAvatars = qobject_cast<IAvatars *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IStatusChanger").value(0,NULL);
	if (plugin)
		FStatusChanger = qobject_cast<IStatusChanger *>(plugin->instance());

	return FNotifications!=NULL && FAvatars!=NULL && FStatusChanger!=NULL;
}

bool UnityIntegration::initObjects()
{
	FNotificationAllowTypes <<  NNT_CHAT_MESSAGE << NNT_NORMAL_MESSAGE << NNT_MUC_MESSAGE_GROUPCHAT << NNT_MUC_MESSAGE_PRIVATE << NNT_MUC_MESSAGE_MENTION;

#ifdef MESSAGING_MENU
	FMessagingMenu = new MessagingMenu::Application("vacuum.desktop");	connect(FMessagingMenu,SIGNAL(sourceActivated(MessagingMenu::Application::Source&)),this,SLOT(onSourceActivated(MessagingMenu::Application::Source&)));
	connect(FMessagingMenu,SIGNAL(statusChanged(MessagingMenu::Status)),this,SLOT(onStatusChanged(MessagingMenu::Status)));
	FMessagingMenu->registerApp();
#endif

	return true;
}

void UnityIntegration::onProfileOpened(const QString &AProfile)
{
	Q_UNUSED(AProfile);

	FLauncherMenu = new Menu;
	if(FMainWindowPlugin)
	{
		FActionRoster = new Action(this);
		FActionRoster->setText(tr("Show roster"));
		connect(FActionRoster,SIGNAL(triggered(bool)),FMainWindowPlugin->instance(),SLOT(onShowMainWindowByAction(bool)));
		FLauncherMenu->addAction(FActionRoster,10,false);
	}
	if(FFileStreamsManager)
	{
		FActionFilesTransferDialog = new Action(this);
		FActionFilesTransferDialog->setText(tr("File Transfers"));
		connect(FActionFilesTransferDialog,SIGNAL(triggered(bool)),FFileStreamsManager->instance(),SLOT(onShowFileStreamsWindow(bool)));
		FLauncherMenu->addAction(FActionFilesTransferDialog,10,false);
	}
	if(FOptionsManager)
	{
		FActionOptionsDialog = new Action(this);
		FActionOptionsDialog->setText(tr("Options"));
		connect(FActionOptionsDialog,SIGNAL(triggered(bool)),FOptionsManager->instance(),SLOT(onShowOptionsDialogByAction(bool)));
		FLauncherMenu->addAction(FActionOptionsDialog,20,false);
	}
	if(FPluginManager)
	{
		FActionPluginsDialog = new Action(this);
		FActionPluginsDialog->setText(tr("Setup plugins"));
		connect(FActionPluginsDialog,SIGNAL(triggered(bool)),FPluginManager->instance(),SLOT(onShowSetupPluginsDialog(bool)));
		FLauncherMenu->addAction(FActionPluginsDialog,20,false);
	}
	if(FOptionsManager)
	{
		FActionChangeProfile = new Action(this);
		FActionChangeProfile->setText(tr("Change Profile"));
		connect(FActionChangeProfile,SIGNAL(triggered(bool)),FOptionsManager->instance(),SLOT(onChangeProfileByAction(bool)));
		FLauncherMenu->addAction(FActionChangeProfile,20,false);
	}

	menu_export = new DBusMenuExporter ("/vacuum", FLauncherMenu);
	sendMessage("quicklist", "/vacuum");
}

void UnityIntegration::updateCount(qint64 FLauncherCount)
{
	if (FLauncherCount <= 99)
	{
		QVariantMap map;
		map.insert(QLatin1String("count"), FLauncherCount);
		map.insert(QLatin1String("count-visible"), FLauncherCount > 0);
		sendMessage(map);
	}
}

void UnityIntegration::onNotificationAdded(int ANotifyId, const INotification &ANotification)
{
	if (FNotificationAllowTypes.contains(ANotification.typeId))
	{
		FNotificationsCount.append(ANotifyId);
		FLauncherCount = FNotificationsCount.count();
		updateCount(FLauncherCount);
	}

#ifdef MESSAGING_MENU
	if (FNotificationAllowTypes.contains(ANotification.typeId))
	{
		Jid contactJid = ANotification.data.value(NDR_CONTACT_JID).toString();
		QString menuEntryId = ANotification.typeId + "|" + contactJid.pFull();

		if(!FMessagingMenu->hasSourceById(menuEntryId))
		{
			MessagingMenu::Application::Source *source = &FMessagingMenu->createSourceTime(menuEntryId, ANotification.data.value(NDR_POPUP_TITLE).toString(),
																						   FAvatars->avatarFileName(FAvatars->avatarHash(contactJid)), QDateTime::currentDateTime(), true);
			source->setAttention(true);
			source->setCount(0);
			FMessagingItems.insert(menuEntryId, source);
		}

		FNotifyIds[menuEntryId].append(ANotifyId);
		FNotifyType.insert(ANotifyId, ANotification.typeId);
		FNotifyJid.insert(ANotifyId, contactJid.pFull());

		if (FNotifyIds.value(menuEntryId).count() > 1)
			FMessagingItems.value(menuEntryId)->setCount(FNotifyIds.value(menuEntryId).count());
	}
#endif
}

void UnityIntegration::onNotificationRemoved(int ANotifyId)
{
	if (FNotificationsCount.contains(ANotifyId))
	{
		FNotificationsCount.removeAll(ANotifyId);
		FLauncherCount = FNotificationsCount.count();
		updateCount(FLauncherCount);
	}

#ifdef MESSAGING_MENU
	if (FNotificationAllowTypes.contains(FNotifyType.value(ANotifyId)))
	{
		QString contactJid = FNotifyJid.value(ANotifyId);
		QString menuEntryId = FNotifyType.value(ANotifyId) + "|" + contactJid;

		FNotifyIds[menuEntryId].removeAll(ANotifyId);
		FNotifyType.remove(ANotifyId);
		FNotifyJid.remove(ANotifyId);

		if (FNotifyIds.value(menuEntryId).count() > 0 && FMessagingMenu->hasSourceById(menuEntryId))
		{
			FMessagingItems.value(menuEntryId)->setCount(FNotifyIds.value(menuEntryId).count());
		}
		else if (FMessagingMenu->hasSourceById(menuEntryId))
		{
			FMessagingMenu->removeSource(*FMessagingItems.value(menuEntryId));
			FMessagingItems.remove(menuEntryId);
		}
	}
#endif
}

#ifdef MESSAGING_MENU
void UnityIntegration::onSourceActivated(MessagingMenu::Application::Source &ASource)
{
	foreach (int id, FNotifications->notifications())
	{
		Jid contactJid = FNotifications->notificationById(id).data.value(NDR_CONTACT_JID).toString();
		if (QString(FNotifications->notificationById(id).typeId + "|" +
					contactJid.pFull()) == FMessagingItems.key(&ASource))
			FNotifications->activateNotification(id);
	}
}

void UnityIntegration::onStatusChanged(MessagingMenu::Status AStatus)
{
	int newStatus;
	switch (AStatus)
	{
	case MessagingMenu::Available:
		newStatus = STATUS_ONLINE;
		break;
	case MessagingMenu::Away:
		newStatus = STATUS_AWAY;
		break;
	case MessagingMenu::Busy:
		newStatus = STATUS_DND;
		break;
	case MessagingMenu::Invisible:
		newStatus = STATUS_INVISIBLE;
		break;
	case MessagingMenu::Offline:
		newStatus = STATUS_OFFLINE;
		break;
	default:
		newStatus = STATUS_NULL_ID;
		break;
	}
	FStatusChanger->setMainStatus(newStatus);
}

#endif

void UnityIntegration::onShutdownStarted()
{
	sendMessage("count-visible", false);
	sendMessage("progress-visible", false);
	delete menu_export.data();

#ifdef MESSAGING_MENU
	FMessagingMenu->unregisterApp();
#endif
}

Q_EXPORT_PLUGIN2(plg_unityintegration, UnityIntegration)
