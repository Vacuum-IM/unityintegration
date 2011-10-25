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
#include <interfaces/ifiletransfer.h>
#include <interfaces/imainwindow.h>
#include <interfaces/itraymanager.h>
#include <definitions/optionvalues.h>
#include <utils/options.h>


#ifdef SVNINFO
#  include "svninfo.h"
#else
#  define SVN_REVISION              "0"
#endif

#define UNITYINTEGRATION_UUID  "{60e8e2d3-432a-4b89-95e7-dd8d2102b585}"

class QMenu;

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
        void UpdateStreamProgress(IFileStream *AStream);
        template<typename T> void sendMsg(const char *name, const T& val);


protected slots:
        void onNotificationAdded(int ANotifyId, const INotification &ANotification);
        void onNotificationRemoved(int ANotifyId);
        void onStreamCreated(IFileStream *AStream);
        void onStreamDestroyed(IFileStream *AStream);
        void onStreamProgressChanged();
        void onShutdownStarted();


private:
        INotifications *FNotifications;
        IFileStreamsManager  *FFileStreamsManager;
        IMainWindowPlugin *FMainWindowPlugin;
        ITrayManager *FTrayManager;
        int FShowCount;
        double FProgressBar;
        qint64 FPercentOld;
        QList<QString> FNotificationAllowTypes;
        QList<int> FNotificationCount;
        QWeakPointer<DBusMenuExporter> menu_export;
        QDBusInterface *FUnityDetector;


};

#endif // UNITYINTEGRATION_H

