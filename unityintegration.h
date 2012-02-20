#ifndef UnityIntegration_H
#define UnityIntegration_H

#include <QDebug>
#include <QVariant>
#include <QMenu>
#include <dbusmenuexporter.h>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusReply>
#include <QStandardItemModel>

#include <definitions/notificationtypes.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/ifilestreamsmanager.h>
#include <interfaces/inotifications.h>
#include <interfaces/imultiuserchat.h>
#include <interfaces/imainwindow.h>
#include <interfaces/ixmppstreams.h>
#include <interfaces/ioptionsmanager.h>
#include <definitions/optionvalues.h>
#include <utils/action.h>
#include <utils/options.h>

#define UNITYINTEGRATION_UUID  "{60e8e2d3-432a-4b89-95e7-dd8d2102b585}"

class UnityIntegration :
                        public QObject,
                        public IPlugin
{
        Q_OBJECT;
        Q_INTERFACES(IPlugin);
public:
        UnityIntegration();
        ~UnityIntegration();
        //IPlugin
        virtual QObject *instance() { return this; }
        virtual QUuid pluginUuid() const { return UNITYINTEGRATION_UUID; }
        virtual void pluginInfo(IPluginInfo *APluginInfo);
        virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
        virtual bool initObjects();
        virtual bool initSettings();
        virtual bool startPlugin() { return true; }

protected:
        void showCount(int FUicount);
        template<typename T> void sendMsg(const char *name, const T& val);


protected slots:
        void onNotificationAdded(int ANotifyId, const INotification &ANotification);
        void onNotificationRemoved(int ANotifyId);
        void onShutdownStarted();


private:
        INotifications *FNotifications;
        IMultiUserChatPlugin *FMultiUserChatPlugin;
        IFileStreamsManager *FFileStreamsManager;
        IMainWindowPlugin *FMainWindowPlugin;
        IPluginManager *FPluginManager;
        IOptionsManager *FOptionsManager;

        qint64 FShowCount;
        QList<QString> FNotificationAllowTypes;
        QList<int> FNotificationCount;
        Menu *FUnityMenu;

        Action *FShowRoster;
        Action *FShowConferences;
        Action *FFilesTransferDialog;
        Action *FChangeProfileAction;
        Action *FShowOptionsDialogAction;
        Action *FPluginsDialog;
        Action *FQuitAction;

        QWeakPointer<DBusMenuExporter> menu_export;
        QDBusInterface *FUnityDetector;
        QDBusInterface *FThirdUnityInterfaceDetector;

};

#endif // UNITYINTEGRATION_H

