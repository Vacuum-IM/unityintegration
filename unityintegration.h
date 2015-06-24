#ifndef UNITYINTEGRATION_H
#define UNITYINTEGRATION_H

#include <QVariant>
#include <QMenu>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusReply>

#include <dbusmenu-qt/dbusmenuexporter.h>

#ifdef MESSAGING_MENU
#include "thirdparty/messaging-menu-qt/messaging-menu-qt.hpp"
#endif

#include <definitions/notificationdataroles.h>
#include <definitions/notificationtypes.h>
#include <definitions/optionvalues.h>
#include <interfaces/iavatars.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/ifilestreamsmanager.h>
#include <interfaces/inotifications.h>
#include <interfaces/imultiuserchat.h>
#include <interfaces/imainwindow.h>
#include <interfaces/ioptionsmanager.h>
#include <interfaces/istatuschanger.h>
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
	void updateCount(qint64 FLauncherCount);
	template<typename T> void sendMessage(const char *name, const T& val);
	void sendMessage(const QVariantMap &properties);

protected slots:
	void onNotificationAdded(int ANotifyId, const INotification &ANotification);
	void onNotificationRemoved(int ANotifyId);
	void onProfileOpened(const QString &AProfile);
#ifdef MESSAGING_MENU
	void onSourceActivated(MessagingMenu::Application::Source &ASource);
	void onStatusChanged(MessagingMenu::Status AStatus);
#endif
	void onShutdownStarted();

private:
	IFileStreamsManager *FFileStreamsManager;
	IMainWindowPlugin *FMainWindowPlugin;
	IMultiUserChatManager *FMultiUserChatManager;
	INotifications *FNotifications;
	IOptionsManager *FOptionsManager;
	IPluginManager *FPluginManager;
	IStatusChanger *FStatusChanger;
	IAvatars *FAvatars;

	QList<QString> FNotificationAllowTypes;
	QList<int> FNotificationsCount;
	qint64 FLauncherCount;
	Menu *FLauncherMenu;

	Action *FActionChangeProfile;
	Action *FActionFilesTransferDialog;
	Action *FActionPluginsDialog;
	Action *FActionQuit;
	Action *FActionOptionsDialog;
	Action *FActionRoster;

	QWeakPointer<DBusMenuExporter> menu_export;
	QDBusInterface *FUnityInterface;

#ifdef MESSAGING_MENU
	MessagingMenu::Application *FMessagingMenu;

	QHash<QString, MessagingMenu::Application::Source*> FMessagingItems;
	QHash<QString, QList<int> > FNotifyIds;
	QHash<int, QString> FNotifyType;
	QHash<int, QString> FNotifyJid;
#endif

};

#endif // UNITYINTEGRATION_H
