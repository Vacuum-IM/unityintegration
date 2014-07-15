#include "unityintegration.h"
#include <QDebug>

UnityIntegration::UnityIntegration()
{
	FLauncherCount = 0;
	FFileStreamsManager = NULL;
	FMainWindowPlugin = NULL;
	FMultiUserChatPlugin = NULL;
	FNotifications = NULL;
	FOptionsManager = NULL;
	FPluginManager = NULL;
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
		qDebug() << "Unable to send message";
}

UnityIntegration::~UnityIntegration()
{
}

void UnityIntegration::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Unity Integration");
	APluginInfo->description = tr("Provides integration with Dash panel Unity");
	APluginInfo->version = "1.2";
	APluginInfo->author = "Alexey Ivanov";
	APluginInfo->homePage = "http://www.vacuum-im.org";
	APluginInfo->dependences.append(NOTIFICATIONS_UUID);
}

bool UnityIntegration::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	AInitOrder=1000;
	FUnityInterface = new QDBusInterface("com.canonical.Unity","/com/canonical/Unity/Launcher","org.freedesktop.DBus.Properties",QDBusConnection::sessionBus());
	FThirdUnityInterface = new QDBusInterface("com.canonical.Unity","/Unity","org.freedesktop.DBus.Properties",QDBusConnection::sessionBus());

	if((FUnityInterface->lastError().type() != QDBusError::NoError) & (FThirdUnityInterface->lastError().type() != QDBusError::NoError))
	{
		qWarning() << "DBus Unity Launcher API Detector: Probably you are not using Unity now or you do not have applications provide Launcher API. Unloading plugin...";
		return false;
	}

	FPluginManager = APluginManager;

	connect(FPluginManager->instance(),SIGNAL(shutdownStarted()),this,SLOT(onShutdownStarted()));

	IPlugin *plugin = APluginManager->pluginInterface("IMultiUserChatPlugin").value(0,NULL);
	if (plugin)
		FMultiUserChatPlugin = qobject_cast<IMultiUserChatPlugin *>(plugin->instance());
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
		{
			connect(FOptionsManager->instance(),SIGNAL(profileOpened(const QString)),this,SLOT(onProfileOpened(const QString)));
		}
	plugin = APluginManager->pluginInterface("IFileStreamsManager").value(0,NULL);
	if (plugin)
		FFileStreamsManager = qobject_cast<IFileStreamsManager *>(plugin->instance());
	plugin = APluginManager->pluginInterface("IAvatars").value(0,NULL);
	if (plugin)
		FAvatars = qobject_cast<IAvatars *>(plugin->instance());

	return FNotifications!=NULL && FMultiUserChatPlugin!=NULL;
}

bool UnityIntegration::initObjects()
{
	FNotificationAllowTypes << NNT_CAPTCHA_REQUEST << NNT_CHAT_MESSAGE << NNT_NORMAL_MESSAGE << NNT_FILETRANSFER << NNT_MUC_MESSAGE_INVITE << NNT_MUC_MESSAGE_GROUPCHAT << NNT_MUC_MESSAGE_PRIVATE << NNT_MUC_MESSAGE_MENTION << NNT_SUBSCRIPTION_REQUEST;
#ifdef MESSAGING_MENU
	FMessagingMenuAllowTypes << NNT_CHAT_MESSAGE << NNT_NORMAL_MESSAGE << NNT_MUC_MESSAGE_GROUPCHAT << NNT_MUC_MESSAGE_PRIVATE;
#endif

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
		FLauncherMenu->addAction(FActionOptionsDialog,11,false);
	}
	if(FPluginManager)
	{
		FActionPluginsDialog = new Action(this);
		FActionPluginsDialog->setText(tr("Setup plugins"));
		connect(FActionPluginsDialog,SIGNAL(triggered(bool)),FPluginManager->instance(),SLOT(onShowSetupPluginsDialog(bool)));
		FLauncherMenu->addAction(FActionPluginsDialog,11,false);
	}
	if(FOptionsManager)
	{
		FActionChangeProfile = new Action(this);
		FActionChangeProfile->setText(tr("Change Profile"));
		connect(FActionChangeProfile,SIGNAL(triggered(bool)),FOptionsManager->instance(),SLOT(onChangeProfileByAction(bool)));
		FLauncherMenu->addAction(FActionChangeProfile,12,false);
	}
	if(FPluginManager)
	{
		FActionQuit = new Action(this);
		FActionQuit->setText(tr("Quit"));
		connect(FActionQuit,SIGNAL(triggered(bool)), FPluginManager->instance(),SLOT(quit()));
		FLauncherMenu->addAction(FActionQuit,12,false);
	}

#ifdef MESSAGING_MENU
	FMessagingMenu = new MessagingMenu::Application("vacuum.desktop");
	connect(FMessagingMenu,SIGNAL(sourceActivated(MessagingMenu::Application::Source&)),this,SLOT(onSourceActivated(MessagingMenu::Application::Source&)));
	FMessagingMenu->registerApp();
#endif

	return true;
}

void UnityIntegration::onProfileOpened(const QString &AProfile)
{
	Q_UNUSED(AProfile);
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
	if (FMessagingMenuAllowTypes.contains(ANotification.typeId))
	{
		//Jid streamJid = ANotification.data.value(NDR_STREAM_JID).toString();
		Jid contactJid = ANotification.data.value(NDR_CONTACT_JID).toString();

		if(!FMessagingMenu->hasSourceById(contactJid.pFull()))
		{
			MessagingMenu::Application::Source *source = &FMessagingMenu->createSourceTime(contactJid.pFull(), ANotification.data.value(NDR_POPUP_TITLE).toString(),
																						   FAvatars->avatarFileName(FAvatars->avatarHash(contactJid)), QDateTime::currentDateTime(), true);
			source->setAttention(true);
			FMessagingItems.insert(contactJid.pFull(), source);
		}

		FNotifyIds[contactJid.pFull()].append(ANotifyId);
		FNotifyType.insert(ANotifyId, ANotification.typeId);
		FNotifyJid.insert(ANotifyId, contactJid.pFull());

		if (FNotifyIds.value(contactJid.pFull()).count() > 1)
			FMessagingItems.value(contactJid.pFull())->setCount(FNotifyIds.value(contactJid.pFull()).count());
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
	if (FMessagingMenuAllowTypes.contains(FNotifyType.value(ANotifyId)))
	{
		QString contactJid = FNotifyJid.value(ANotifyId);

		FNotifyIds[contactJid].removeAll(ANotifyId);
		FNotifyType.remove(ANotifyId);
		FNotifyJid.remove(ANotifyId);

		if (FNotifyIds.value(contactJid).count() > 0 && FMessagingMenu->hasSourceById(contactJid))
		{
			FMessagingItems.value(contactJid)->setCount(FNotifyIds.value(contactJid).count());
		}
		else if (FMessagingMenu->hasSourceById(contactJid))
		{
			FMessagingMenu->removeSource(*FMessagingItems.value(contactJid));
			FMessagingItems.remove(contactJid);
		}
	}
#endif
}

#ifdef MESSAGING_MENU
void UnityIntegration::onSourceActivated(MessagingMenu::Application::Source &ASource)
{
	foreach (int id, FNotifications->notifications())
	{
		if (FNotifications->notificationById(id).data.value(NDR_CONTACT_JID).toString() == FMessagingItems.key(&ASource) &&
			FMessagingMenuAllowTypes.contains(FNotifications->notificationById(id).typeId))
		{
			FNotifications->activateNotification(id);
		}
	}
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
