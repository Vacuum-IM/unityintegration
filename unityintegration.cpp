#include "unityintegration.h"

UnityIntegration::UnityIntegration()
{
        FShowCount = 0;
        FPercentOld = 0;
}

UnityIntegration::~UnityIntegration()
{
}

void UnityIntegration::pluginInfo(IPluginInfo *APluginInfo)
{
        APluginInfo->name = tr("Unity Integration");
        APluginInfo->description = tr("Provides integration with Dash panel Unity");
        APluginInfo->version = "0.1."SVN_REVISION;
        APluginInfo->author = "Alexey Ivanov";
        APluginInfo->homePage = "http://www.vacuum-im.org";
        APluginInfo->dependences.append(NOTIFICATIONS_UUID);
        APluginInfo->dependences.append(FILESTREAMSMANAGER_UUID);
        APluginInfo->dependences.append(MAINWINDOW_UUID);
}

bool UnityIntegration::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{

    FUnityDetector = new QDBusInterface("com.canonical.Unity","/com/canonical/Unity","com.canonical.Unity",QDBusConnection::sessionBus());
            if(FUnityDetector->lastError().type() != QDBusError::NoError)
            {
                   qWarning() << "DBus Ubuntu Unity Detector: Probably you are not using Unity now. Unloading plugin...";
                   return false;
            }

         qDebug() << "Vacuum IM Unity Integration started...";

        IPlugin *plugin = APluginManager->pluginInterface("IFileStreamsManager").value(0,NULL);
        if (plugin)
            {
                FFileStreamsManager = qobject_cast<IFileStreamsManager *>(plugin->instance());
                if (FFileStreamsManager)
                        {
                        connect(FFileStreamsManager->instance(),SIGNAL(streamCreated(IFileStream *)),SLOT(onStreamCreated(IFileStream *)));
                        connect(FFileStreamsManager->instance(),SIGNAL(streamDestroyed(IFileStream *)),SLOT(onStreamDestroyed(IFileStream *)));
                        }
            }

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


        connect(APluginManager->instance(),SIGNAL(shutdownStarted()),SLOT(onShutdownStarted()));

        return FFileStreamsManager!=NULL && FNotifications!=NULL && FMainWindowPlugin!=NULL;;
}

bool UnityIntegration::initObjects()
{
        FNotificationAllowTypes << NNT_CAPTCHA_REQUEST << NNT_CHAT_MESSAGE << NNT_NORMAL_MESSAGE << NNT_FILETRANSFER << NNT_MUC_MESSAGE_INVITE << NNT_MUC_MESSAGE_GROUPCHAT << NNT_MUC_MESSAGE_PRIVATE << NNT_MUC_MESSAGE_MENTION << NNT_SUBSCRIPTION_REQUEST << NNT_BIRTHDAY;

        menu_export = new DBusMenuExporter ("/vacuum", FMainWindowPlugin->mainWindow()->mainMenu());
        sendMsg("quicklist", "/vacuum");

        return true;
}

bool UnityIntegration::initSettings()
{
        return true;
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


void UnityIntegration::showCount(int FShowCount)
{

    if (FShowCount != 0)
        {
            sendMsg("count", qint64(FShowCount));
            sendMsg("count-visible", true);
            //sendMsg("quicklist", "");
        }
    else
        sendMsg("count-visible", false);
}



void UnityIntegration::onNotificationAdded(int ANotifyId, const INotification &ANotification)
{
    //INotification notify = FNotifications->notificationById(ANotifyId);

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


void UnityIntegration::onStreamCreated(IFileStream *AStream)
{
    connect(AStream->instance(), SIGNAL(progressChanged()),SLOT(onStreamProgressChanged()));

    UpdateStreamProgress(AStream);
}

void UnityIntegration::onStreamDestroyed(IFileStream *AStream)
{

}

void UnityIntegration::onStreamProgressChanged()
{
        IFileStream *stream = qobject_cast<IFileStream *>(sender());

        if (stream)
                UpdateStreamProgress(stream);
}

void UnityIntegration::UpdateStreamProgress(IFileStream *AStream)
{

            qint64 minPos = AStream->rangeOffset();
            qint64 maxPos = AStream->rangeLength()>0 ? AStream->rangeLength()+AStream->rangeOffset() : AStream->fileSize();
            qint64 percent = maxPos>0.00 ? ((minPos+AStream->progress())*100)/maxPos : 0;

            if ((percent - FPercentOld) >=1 )
            {
                if ((percent != 0) && (percent != 100))
                       {
                                sendMsg("progress", static_cast<double>(percent)/100.0);
                                sendMsg("progress-visible", true);
                       }
                        else
                              {sendMsg("progress-visible", false);}

            FPercentOld = percent;
            }
}

void UnityIntegration::onShutdownStarted()
{
    qDebug() << "Vacuum IM Unity Integration shutdown...";
    sendMsg("count-visible", false);
    sendMsg("progress-visible", false);
    sendMsg("quicklist", "/vacuum-menu-remove");



}

Q_EXPORT_PLUGIN2(plg_unityintegration, UnityIntegration)

