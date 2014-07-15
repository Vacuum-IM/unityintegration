/****************************************************************************
 * messaging-menu.cpp
 *  Copyright © 2012, Vsevolod Velichko <torkvema@gmail.com>.
 *  Licence: GPLv3 or later
 *
 ****************************************************************************
 *                                                                          *
 *   This library is free software; you can redistribute it and/or modify   *
 *   it under the terms of the GNU General Public License as published by   *
 *   the Free Software Foundation; either version 3 of the License, or      *
 *   (at your option) any later version.                                    *
 *                                                                          *
 ****************************************************************************/

#define QT_NO_KEYWORDS // Otherwise it'll break GTK
#include "messaging-menu-qt.hpp"
#include <messaging-menu/messaging-menu.h>
#include <QSharedPointer>
#include <QHash>

class GIconPtr
{
	private:
		GIcon *icon;
	public:
		GIconPtr() : icon(NULL) {}
		GIconPtr(const QString &_icon = QString()) : icon(NULL)
		{
			if (!_icon.isNull())
			{
				GFile *file = g_file_new_for_path(_icon.toUtf8().constData());
				icon = g_file_icon_new(file);
			}
		}
		GIcon* operator*()
		{
			return icon;
		}
		~GIconPtr()
		{
			if(icon)
			g_object_unref (icon);
		}
};

using namespace MessagingMenu;
namespace MessagingMenu
{
	typedef enum
	{
		SourceTime,
		SourceCount,
		SourceString
	} SourceType;

	class Application::SourcePrivate
	{
	public:
		const QString id;
		QString label;
		QString icon;
		QPair<bool, quint32> count;
		QPair<bool, qint64> time;
		QPair<bool, QString> str;
		bool attention;
		Application *application;
		SourceType type;
		bool persistent;
		qint32 position;
	public:
		SourcePrivate() = delete;
		SourcePrivate (const QString &_id, const QString &_label, const QString &_icon, Application *_application)
			: id (_id)
			, label (_label)
			, icon (_icon)
			, count (false, 0U)
			, time (false, 0L)
			, str (false, QString() )
			, attention (false)
			, application (_application)
			, type(SourceTime)
			, persistent(false)
			, position(0)
		{}

		MessagingMenuApp *getApp() const;

		void sendCount() {
			if (count.first) {
				messaging_menu_app_set_source_count (getApp(), id.toUtf8().constData(), count.second);
			}
		}

		void sendTime() {
			if (time.first) {
				messaging_menu_app_set_source_time (getApp(), id.toUtf8().constData(), time.second);
			}
		}

		void sendString() {
			if (!str.first) {
				return;
			}
			messaging_menu_app_set_source_string (getApp(), id.toUtf8().constData(), str.second.toUtf8().constData());
		}

		void sendLabel() {
			messaging_menu_app_set_source_label (getApp(), id.toUtf8().constData(), label.toUtf8().constData());
		}

		void sendIcon() {
			GIconPtr i(icon);
			messaging_menu_app_set_source_icon (getApp(), id.toUtf8().constData(), *i);
		}

		void sendAttention() {
			if (attention) {
				messaging_menu_app_draw_attention (getApp(), id.toUtf8().constData());
			} else {
				messaging_menu_app_remove_attention (getApp(), id.toUtf8().constData());
			}
		}
	};

	class ApplicationPrivate
	{
	public:
		MessagingMenuApp *app;
		QHash<QString, QSharedPointer<Application::Source> > sources;
		ApplicationPrivate()
			: app(NULL)
			, sources()
		{
		}
		static void emitStatusChanged(MessagingMenuApp *, MessagingMenuStatus st, gpointer user_data)
		{
			Status status = static_cast<Status>(static_cast<int>(st));
			Application *a = reinterpret_cast<Application*>(user_data);
			Q_EMIT a->statusChanged(status);
		}

		static void emitSourceActivated(MessagingMenuApp *, const gchar *id, gpointer user_data)
		{
			Application *a = reinterpret_cast<Application*>(user_data);
			QString qId = QString::fromUtf8(id);
			auto it = a->d_ptr->sources.find(qId);
			if (it == a->d_ptr->sources.end())
				return;
			QSharedPointer<Application::Source> sourcePtr = *it;
			Application::Source& source = *sourcePtr;
			qint32 position = source.getPosition();
			if (source.isPersistent())
			{
				a->d_ptr->sendCreatedSource(source, position);
			}
			else
			{
				a->d_ptr->sources.remove (source.getId().toUtf8().constData());
				a->d_ptr->shiftPositions(position, -1);
			}
			Q_EMIT a->sourceActivated(source);
		}

		void shiftPositions(qint32 position, qint32 shift = 1)
		{
			if (position >= 0)
				for (auto it = sources.begin(); it != sources.end(); ++it)
					if ((*it)->getPosition() >= position)
						(*it)->setPosition((*it)->getPosition() + shift);
		}

		Application::Source &createSource (Application *parent, const QString &id, const QString &label, const QString &icon, bool persistent, qint32 position)
		{
			shiftPositions(position);
			Application::Source &source = *sources.insert (id, QSharedPointer<Application::Source> (new Application::Source (parent, id, label, icon) ) ).value();
			source.setPersistent(persistent);
			if (position >= 0) {
				source.setPosition(position);
			} else {
				source.setPosition(sources.size() - 1);
			}
			return source;
		}

		inline void sendCreatedSource(Application::Source &source, qint32 position)
		{
			GIconPtr i(source.getIcon());
			QString d = source.getLabel();
			if(position >= 0)
			{
				switch(source.d_ptr->type) {
					case SourceTime:
						messaging_menu_app_insert_source_with_time (app, position, source.getId().toUtf8().constData(), *i, d.toUtf8().constData(), source.d_ptr->time.second);
						break;
					case SourceCount:
						messaging_menu_app_insert_source_with_count (app, position, source.getId().toUtf8().constData(), *i, d.toUtf8().constData(), source.d_ptr->count.second);
						break;
					case SourceString:
						QByteArray str = source.d_ptr->str.second.toUtf8();
						messaging_menu_app_insert_source_with_string (app, position, source.getId().toUtf8().constData(), *i, d.toUtf8().constData(), str.data());
						break;
				}
			} else {
				switch(source.d_ptr->type) {
					case SourceTime:
						messaging_menu_app_append_source_with_time (app, source.getId().toUtf8().constData(), *i, d.toUtf8().constData(), source.d_ptr->time.second);
						break;
					case SourceCount:
						messaging_menu_app_append_source_with_count (app, source.getId().toUtf8().constData(), *i, d.toUtf8().constData(), source.d_ptr->count.second);
						break;
					case SourceString:
						QString str = source.d_ptr->str.second;
						messaging_menu_app_append_source_with_string (app, source.getId().toUtf8().constData(), *i, d.toUtf8().constData(), str.toUtf8().constData());
						break;
				}
			}
			source.sendAll();
		}

	};

	MessagingMenuApp *Application::SourcePrivate::getApp() const
	{
		return application->d_ptr->app;
	}
}

Application::Source::Source (Application *application, const QString &id, const QString &label, const QString &icon)
	: d_ptr (new SourcePrivate (id, label, icon, application) )
{
}

bool Application::Source::hasCount () const
{
	return d_ptr->count.first;
}
bool Application::Source::hasTime() const
{
	return d_ptr->time.first;
}
bool Application::Source::hasStr() const
{
	return d_ptr->str.first;
}
quint32 Application::Source::getCount() const
{
	return d_ptr->count.second;
}
QDateTime Application::Source::getTime() const
{
	return QDateTime::fromMSecsSinceEpoch(d_ptr->time.second / 1000);
}
const QString &Application::Source::getString() const
{
	return d_ptr->str.second;
}
const QString &Application::Source::getLabel() const
{
	return d_ptr->label;
}
const QString &Application::Source::getIcon() const
{
	return d_ptr->icon;
}
const QString &Application::Source::getId() const
{
	return d_ptr->id;
}

void Application::Source::setCount (quint32 count)
{
	d_ptr->count.first = true;
	d_ptr->time.first = false;
	d_ptr->str.first = false;
	d_ptr->count.second = count;
	d_ptr->sendCount();
}

void Application::Source::setTime (const QDateTime &time)
{
	d_ptr->time.first = true;
	d_ptr->str.first = false;
	d_ptr->count.first = false;
	d_ptr->time.second = time.toMSecsSinceEpoch() * 1000;
	d_ptr->sendTime();
}

void Application::Source::setString (const QString &str)
{
	d_ptr->str.first = true;
	d_ptr->time.first = false;
	d_ptr->count.first = false;
	d_ptr->str.second = str;
	d_ptr->sendString();
}

void Application::Source::setLabel (const QString &label)
{
	d_ptr->label = label;
	d_ptr->sendLabel();
}

void Application::Source::setIcon (const QString &icon)
{
	d_ptr->icon = icon;
	d_ptr->sendIcon();
}

void Application::Source::sendAll ()
{
	d_ptr->sendCount();
	d_ptr->sendTime();
	d_ptr->sendString();
	d_ptr->sendAttention();
}

void Application::Source::setAttention(bool draw)
{
	d_ptr->attention = draw;
	d_ptr->sendAttention();
}

void Application::Source::setPosition(qint32 position)
{
	d_ptr->position = position;
}

qint32 Application::Source::getPosition() const
{
	return d_ptr->position;
}

bool Application::Source::isPersistent() const
{
	return d_ptr->persistent;
}

void Application::Source::setPersistent(bool persistent)
{
	d_ptr->persistent = persistent;
}

Application::Application (const QString &desktopId, QObject *parent)
	: QObject (parent)
	, d_ptr (new ApplicationPrivate)
{
	g_type_init();
	QByteArray d = desktopId.toUtf8();
	d_ptr->app = messaging_menu_app_new (d.data() );
	g_signal_connect(d_ptr->app, "activate-source", G_CALLBACK(ApplicationPrivate::emitSourceActivated), this);
	g_signal_connect(d_ptr->app, "status-changed", G_CALLBACK(ApplicationPrivate::emitStatusChanged), this);
}

Application::~Application()
{
	delete d_ptr;
}

void Application::registerApp()
{
	messaging_menu_app_register (d_ptr->app);
}

void Application::unregisterApp()
{
	messaging_menu_app_unregister (d_ptr->app);
}

void Application::setStatus (Status status)
{
	messaging_menu_app_set_status (d_ptr->app, static_cast<MessagingMenuStatus> (static_cast<int> (status) ) );
}

Application::Source &Application::createSourceTime (const QString &id, const QString &label, const QString &icon, const QDateTime &time, bool persistent, qint32 position)
{
	Source &source = d_ptr->createSource(this, id, label, icon, persistent, position);
	source.d_ptr->time.first = true;
	source.d_ptr->time.second = time.toMSecsSinceEpoch() * 1000;
	source.d_ptr->type = SourceTime;
	d_ptr->sendCreatedSource(source, position);
	return source;
}
Application::Source &Application::createSourceCount (const QString &id, const QString &label, const QString &icon, quint32 count, bool persistent, qint32 position)
{
	Source &source = d_ptr->createSource(this, id, label, icon, persistent, position);
	source.d_ptr->count.first = true;
	source.d_ptr->count.second = count;
	source.d_ptr->type = SourceCount;
	d_ptr->sendCreatedSource(source, position);
	return source;
}
Application::Source &Application::createSourceString (const QString &id, const QString &label, const QString &icon, const QString &str, bool persistent, qint32 position)
{
	Source &source = d_ptr->createSource(this, id, label, icon, persistent, position);
	source.d_ptr->str.first = true;
	source.d_ptr->str.second = str;
	source.d_ptr->type = SourceString;
	d_ptr->sendCreatedSource(source, position);
	return source;
}

void Application::removeSource (Source &source)
{
	messaging_menu_app_remove_source (d_ptr->app, source.getId().toUtf8().constData() );
	qint32 position = source.getPosition();
	d_ptr->sources.remove (source.getId().toUtf8().constData());
	d_ptr->shiftPositions(position, -1);
}

bool Application::hasSource (const Source &source)
{
	return messaging_menu_app_has_source (d_ptr->app, source.getId().toUtf8().constData() );
}

bool Application::hasSourceById (const QString &id)
{
	return messaging_menu_app_has_source (d_ptr->app, id.toUtf8().constData() );
}
