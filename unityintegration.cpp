#include "unityintegration.h"

UnityIntegration::UnityIntegration()
{
        FShowCount = 0;
}

template<typename T> void UnityIntegration::sendMsg(const char *name, const T& val)
{
        QDBusMessage message = QDBusMessage::createSignal("/vacuum", "com.canonical.Unity.LauncherEntry", "Update");
        QVariantList args;
        QVariantMap map;
        map.insert(QLatin1String(name), val);
        args << QLatin1String("application://vacuum.desktop") << map;
        message.setArguments(args);
        QDBusConnection::sessionBus().send(message);
}

UnityIntegration::~UnityIntegration()
{
}

void UnityIntegration::pluginInfo(IPluginInfo *APluginInfo)
{
        APluginInfo->name = tr("Unity Integration");
        APluginInfo->description = tr("Provides integration with Dash panel Unity");
        APluginInfo->version = "0.2";
        APluginInfo->author = "Alexey Ivanov";
        APluginInfo->homePage = "http://www.vacuum-im.org";
        APluginInfo->dependences.append(NOTIFICATIONS_UUID);
        APluginInfo->dependences.append(FILESTREAMSMANAGER_UUID);
        APluginInfo->dependences.append(MAINWINDOW_UUID);
}

bool UnityIntegration::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
        //Q_UNUSED(AInitOrder);
        AInitOrder=1000;

        FUnityDetector = new QDBusInterface("com.canonical.Unity","/com/canonical/Unity/Launcher","org.freedesktop.DBus.Properties",QDBusConnection::sessionBus());
        FThirdUnityInterfaceDetector = new QDBusInterface("com.canonical.Unity","/Unity","org.freedesktop.DBus.Properties",QDBusConnection::sessionBus());

        if((FUnityDetector->lastError().type() != QDBusError::NoError) & (FThirdUnityInterfaceDetector->lastError().type() != QDBusError::NoError))
        {
                qWarning() << "DBus Unity Launcher API Detector: Probably you are not using Unity now or you do not have applications provide Launcher API. Unloading plugin...";
                return false;
        }

         qDebug() << "Vacuum IM Unity Integration started...";

        FPluginManager = APluginManager;

        IPlugin *plugin = APluginManager->pluginInterface("IFileStreamsManager").value(0,NULL);
        if (plugin)

                FFileStreamsManager = qobject_cast<IFileStreamsManager *>(plugin->instance());

        plugin = APluginManager->pluginInterface("INotifications").value(0,NULL);
        if (plugin)
        {
                FNotifications = qobject_cast<INotifications *>(plugin->instance());
                if (FNotifications)
                        {
                        connect (FNotifications->instance(), SIGNAL(notificationAppended(int, const INotification)),this,SLOT(onNotificationAdded(int, const INotification)));
                        connect (FNotifications->instance(), SIGNAL(notificationRemoved(int)),this,SLOT(onNotificationRemoved(int)));
                        }
        }

        plugin = APluginManager->pluginInterface("IMainWindowPlugin").value(0,NULL);
                if (plugin)
                        FMainWindowPlugin = qobject_cast<IMainWindowPlugin *>(plugin->instance());

        plugin = APluginManager->pluginInterface("ITrayManager").value(0,NULL);
                if (plugin)
                        FTrayManager = qobject_cast<ITrayManager *>(plugin->instance());

        plugin = APluginManager->pluginInterface("IStatusChanger").value(0,NULL);
                if (plugin)
                        FStatusChanger = qobject_cast<IStatusChanger *>(plugin->instance());

        plugin = APluginManager->pluginInterface("IOptionsManager").value(0,NULL);
                if (plugin)
                        FOptionsManager = qobject_cast<IOptionsManager *>(plugin->instance());


        connect(APluginManager->instance(),SIGNAL(shutdownStarted()),SLOT(onShutdownStarted()));

        return FFileStreamsManager!=NULL && FNotifications!=NULL && FMainWindowPlugin!=NULL;;
}

bool UnityIntegration::initObjects()
{
        FNotificationAllowTypes << NNT_CAPTCHA_REQUEST << NNT_CHAT_MESSAGE << NNT_NORMAL_MESSAGE << NNT_FILETRANSFER << NNT_MUC_MESSAGE_INVITE << NNT_MUC_MESSAGE_GROUPCHAT << NNT_MUC_MESSAGE_PRIVATE << NNT_MUC_MESSAGE_MENTION << NNT_SUBSCRIPTION_REQUEST << NNT_BIRTHDAY;

        FUnityMenu = new Menu;

        FShowRoster = new Action(this);
        FShowRoster->setText(tr("Show roster"));
        connect(FShowRoster,SIGNAL(triggered(bool)),FMainWindowPlugin->instance(),SLOT(onShowMainWindowByAction(bool)));

        FFilesTransferDialog = new Action(this);
        FFilesTransferDialog->setText(tr("File Transfers"));
        connect(FFilesTransferDialog,SIGNAL(triggered(bool)),FFileStreamsManager->instance(),SLOT(onShowFileStreamsWindow(bool)));

        FPluginsDialog = new Action(this);
        FPluginsDialog->setText(tr("Setup plugins"));
        connect(FPluginsDialog,SIGNAL(triggered(bool)),FPluginManager->instance(),SLOT(onShowSetupPluginsDialog(bool)));

        FChangeProfileAction = new Action(this);
        FChangeProfileAction->setText(tr("Change Profile"));
        connect(FChangeProfileAction,SIGNAL(triggered(bool)),FOptionsManager->instance(),SLOT(onChangeProfileByAction(bool)));

        FShowOptionsDialogAction = new Action(this);
        FShowOptionsDialogAction->setText(tr("Options"));
        connect(FShowOptionsDialogAction,SIGNAL(triggered(bool)),FOptionsManager->instance(),SLOT(onShowOptionsDialogByAction(bool)));

        FQuitAction = new Action(this);
        FQuitAction->setText(tr("Quit"));
        connect(FQuitAction,SIGNAL(triggered(bool)), FPluginManager->instance(),SLOT(quit()));

        signalMapper = new QSignalMapper(this);

        FSetStatusOnline = new Action(this);
        FSetStatusOnline->setText(tr("Online"));
        connect(FSetStatusOnline, SIGNAL(triggered()),signalMapper, SLOT (map()));

        FSetStatusChat = new Action(this);
        FSetStatusChat->setText(tr("Chat"));
        connect(FSetStatusChat, SIGNAL(triggered()),signalMapper, SLOT (map()));

        FSetStatusAway = new Action(this);
        FSetStatusAway->setText(tr("Away"));
        connect(FSetStatusAway, SIGNAL(triggered()),signalMapper, SLOT (map()));

        FSetStatusDND = new Action(this);
        FSetStatusDND->setText(tr("Do not disturb"));
        connect(FSetStatusDND, SIGNAL(triggered()),signalMapper, SLOT (map()));

        FSetStatusExAway = new Action(this);
        FSetStatusExAway->setText(tr("Extended Away"));
        connect(FSetStatusExAway, SIGNAL(triggered()),signalMapper, SLOT (map()));

        FSetStatusInvisible = new Action(this);
        FSetStatusInvisible->setText(tr("Invisible"));
        connect(FSetStatusInvisible, SIGNAL(triggered()),signalMapper, SLOT (map()));


        signalMapper->setMapping(FSetStatusOnline, int(10));
        signalMapper->setMapping(FSetStatusChat, int(15));
        signalMapper->setMapping(FSetStatusAway, int(20));
        signalMapper->setMapping(FSetStatusExAway, int(25));
        signalMapper->setMapping(FSetStatusDND, int(30));
        signalMapper->setMapping(FSetStatusInvisible, int(35));
        connect(signalMapper, SIGNAL(mapped(const int &)), this, SLOT(onStatusChange(const int &)));

        FUnityMenu->addAction(FSetStatusOnline,5048,false);
        FUnityMenu->addAction(FSetStatusChat,5048,false);
        FUnityMenu->addAction(FSetStatusAway,5048,false);
        FUnityMenu->addAction(FSetStatusDND,5048,false);
        FUnityMenu->addAction(FSetStatusExAway,5048,false);
        FUnityMenu->addAction(FSetStatusInvisible,5048,false);
        FUnityMenu->addAction(FShowRoster,5049,false);
        FUnityMenu->addAction(FFilesTransferDialog,5500,false);
        FUnityMenu->addAction(FShowOptionsDialogAction,5500,false);
        FUnityMenu->addAction(FPluginsDialog,5500,false);
        FUnityMenu->addAction(FChangeProfileAction,5501,false);
        FUnityMenu->addAction(FQuitAction,5501,false);

        menu_export = new DBusMenuExporter ("/vacuum", FUnityMenu);
        sendMsg("quicklist", "/vacuum");

        return true;
}

void  UnityIntegration::onStatusChange(const int &StatusNumber)
{
        FStatusChanger->setMainStatus(StatusNumber);
}

bool UnityIntegration::initSettings()
{
        return true;
}


void UnityIntegration::showCount(int FShowCount)
{
        sendMsg("count", static_cast<qint64>(FShowCount));
        sendMsg("count-visible", FShowCount != 0);
}



void UnityIntegration::onNotificationAdded(int ANotifyId, const INotification &ANotification)
{

        if (FNotificationAllowTypes.contains(ANotification.typeId))
        {
                FNotificationCount.append(ANotifyId);
                FShowCount = FNotificationCount.count();
                showCount(FShowCount);
        }
}

void UnityIntegration::onNotificationRemoved(int ANotifyId)
{
        if (FNotificationCount.contains(ANotifyId))
        {
                FNotificationCount.removeAll(ANotifyId);
                FShowCount = FNotificationCount.count();
                showCount(FShowCount);
        }
}


void UnityIntegration::onShutdownStarted()
{
        qDebug() << "Vacuum IM Unity Integration shutdown...";
        sendMsg("count-visible", false);
        sendMsg("progress-visible", false);
        sendMsg("quicklist", "/vacuum-menu-remove"); // that the dirty hack?

}

Q_EXPORT_PLUGIN2(plg_unityintegration, UnityIntegration)

