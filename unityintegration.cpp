#include "unityintegration.h"
#include <QDebug>

UnityIntegration::UnityIntegration()
{
	FCount = 0;
	FFileStreamsManager = NULL;
	FMainWindowPlugin = NULL;
	FMultiUserChatPlugin = NULL;
	FNotifications = NULL;
	FOptionsManager = NULL;
	FPluginManager = NULL;
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
	APluginInfo->version = "1.1";
	APluginInfo->author = "Alexey Ivanov";
	APluginInfo->homePage = "http://www.vacuum-im.org";
	APluginInfo->dependences.append(NOTIFICATIONS_UUID);
}

bool UnityIntegration::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	AInitOrder=1000;
	FUnityDetector = new QDBusInterface("com.canonical.Unity","/com/canonical/Unity/Launcher","org.freedesktop.DBus.Properties",QDBusConnection::sessionBus());
	FThirdUnityInterfaceDetector = new QDBusInterface("com.canonical.Unity","/Unity","org.freedesktop.DBus.Properties",QDBusConnection::sessionBus());

	if((FUnityDetector->lastError().type() != QDBusError::NoError) & (FThirdUnityInterfaceDetector->lastError().type() != QDBusError::NoError))
	{
		qWarning() << "DBus Unity Launcher API Detector: Probably you are not using Unity now or you do not have applications provide Launcher API. Unloading plugin...";
		return false;
	}

	FPluginManager = APluginManager;
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

	return FNotifications!=NULL && FMultiUserChatPlugin!=NULL;
}

bool UnityIntegration::initObjects()
{
	FNotificationAllowTypes << NNT_CAPTCHA_REQUEST << NNT_CHAT_MESSAGE << NNT_NORMAL_MESSAGE << NNT_FILETRANSFER << NNT_MUC_MESSAGE_INVITE << NNT_MUC_MESSAGE_GROUPCHAT << NNT_MUC_MESSAGE_PRIVATE << NNT_MUC_MESSAGE_MENTION << NNT_SUBSCRIPTION_REQUEST;

	FUnityMenu = new Menu;
	if(FMainWindowPlugin)
	{
		FActionRoster = new Action(this);
		FActionRoster->setText(tr("Show roster"));
		connect(FActionRoster,SIGNAL(triggered(bool)),FMainWindowPlugin->instance(),SLOT(onShowMainWindowByAction(bool)));
		FUnityMenu->addAction(FActionRoster,10,false);
	}
	if(FMultiUserChatPlugin)
	{
		FActionConferences = new Action(this);
		FActionConferences->setText(tr("Show all hidden conferences"));
		connect(FActionConferences,SIGNAL(triggered(bool)),FMultiUserChatPlugin->instance(),SLOT(onShowAllRoomsTriggered(bool)));
		FUnityMenu->addAction(FActionConferences,10,false);
	}
	if(FFileStreamsManager)
	{
		FActionFilesTransferDialog = new Action(this);
		FActionFilesTransferDialog->setText(tr("File Transfers"));
		connect(FActionFilesTransferDialog,SIGNAL(triggered(bool)),FFileStreamsManager->instance(),SLOT(onShowFileStreamsWindow(bool)));
		FUnityMenu->addAction(FActionFilesTransferDialog,10,false);
	}
	if(FOptionsManager)
	{
		FActionOptionsDialog = new Action(this);
		FActionOptionsDialog->setText(tr("Options"));
		connect(FActionOptionsDialog,SIGNAL(triggered(bool)),FOptionsManager->instance(),SLOT(onShowOptionsDialogByAction(bool)));
		FUnityMenu->addAction(FActionOptionsDialog,11,false);
	}
	if(FPluginManager)
	{
		FActionPluginsDialog = new Action(this);
		FActionPluginsDialog->setText(tr("Setup plugins"));
		connect(FActionPluginsDialog,SIGNAL(triggered(bool)),FPluginManager->instance(),SLOT(onShowSetupPluginsDialog(bool)));
		FUnityMenu->addAction(FActionPluginsDialog,11,false);
	}
	if(FOptionsManager)
	{
		FActionChangeProfile = new Action(this);
		FActionChangeProfile->setText(tr("Change Profile"));
		connect(FActionChangeProfile,SIGNAL(triggered(bool)),FOptionsManager->instance(),SLOT(onChangeProfileByAction(bool)));
		FUnityMenu->addAction(FActionChangeProfile,12,false);
	}
	if(FPluginManager)
	{
		FActionQuit = new Action(this);
		FActionQuit->setText(tr("Quit"));
		connect(FActionQuit,SIGNAL(triggered(bool)), FPluginManager->instance(),SLOT(quit()));
		FUnityMenu->addAction(FActionQuit,12,false);
	}
	return true;
}

void UnityIntegration::onProfileOpened(const QString &AProfile)
{
	Q_UNUSED(AProfile);
	// Экспортировать меню только после открытия профиля, простая защита от случайного повторного запуска клиента
	menu_export = new DBusMenuExporter ("/vacuum", FUnityMenu);
	sendMessage("quicklist", "/vacuum");
}

void UnityIntegration::showCount(qint64 FCount)
{
	if (FCount <= 99)
	{
		QVariantMap map;
		map.insert(QLatin1String("count"), FCount);
		map.insert(QLatin1String("count-visible"), FCount > 0);
		sendMessage(map);
	}
}

void UnityIntegration::onNotificationAdded(int ANotifyId, const INotification &ANotification)
{
	if (FNotificationAllowTypes.contains(ANotification.typeId))
	{
		FNotificationCount.append(ANotifyId);
		FCount = FNotificationCount.count();
		showCount(FCount);
	}
}

void UnityIntegration::onNotificationRemoved(int ANotifyId)
{
	if (FNotificationCount.contains(ANotifyId))
	{
		FNotificationCount.removeAll(ANotifyId);
		FCount = FNotificationCount.count();
		showCount(FCount);
	}
}

void UnityIntegration::onShutdownStarted()
{
	sendMessage("count-visible", false);
	sendMessage("progress-visible", false);
	delete menu_export.data();
}

Q_EXPORT_PLUGIN2(plg_unityintegration, UnityIntegration)
