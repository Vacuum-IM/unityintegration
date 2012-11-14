#ifndef UnityIntegration_H
#define UnityIntegration_H

#include <QVariant>
#include <QMenu>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusReply>

#include <dbusmenuexporter.h>

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

#define UNITYINTEGRATION_UUID "{60e8e2d3-432a-4b89-95e7-dd8d2102b585}"

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
	virtual bool initSettings() { return true; }
	virtual bool startPlugin() { return true; }

protected:
	void showCount(qint64 FCount);
	template<typename T> void sendMessage(const char *name, const T& val);
	void sendMessage(const QVariantMap &properties);

protected slots:
	void onNotificationAdded(int ANotifyId, const INotification &ANotification);
	void onNotificationRemoved(int ANotifyId);
	void onProfileOpened(const QString &AProfile);
	void onShutdownStarted();

private:
	IFileStreamsManager *FFileStreamsManager;
	IMainWindowPlugin *FMainWindowPlugin;
	IMultiUserChatPlugin *FMultiUserChatPlugin;
	INotifications *FNotifications;
	IOptionsManager *FOptionsManager;
	IPluginManager *FPluginManager;

	qint64 FCount;
	QList<QString> FNotificationAllowTypes;
	QList<int> FNotificationCount;
	Menu *FUnityMenu;

	Action *FActionChangeProfile;
	Action *FActionFilesTransferDialog;
	Action *FActionPluginsDialog;
	Action *FActionQuit;
	Action *FActionConferences;
	Action *FActionOptionsDialog;
	Action *FActionRoster;

	QWeakPointer<DBusMenuExporter> menu_export;
	QDBusInterface *FUnityDetector;
	QDBusInterface *FThirdUnityInterfaceDetector;
};

#endif // UNITYINTEGRATION_H
