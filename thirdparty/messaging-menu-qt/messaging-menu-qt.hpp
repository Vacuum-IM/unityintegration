/****************************************************************************
 * messaging-menu.hpp
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

#ifndef MESSAGING_MENU_30GOW558
#define MESSAGING_MENU_30GOW558

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QDateTime>
#include <QPair>
#include <QIcon>
namespace MessagingMenu
{
    typedef enum
    {
        Available,
        Away,
        Busy,
        Invisible,
        Offline
    } Status;

    class ApplicationPrivate;
    class Application : public QObject
    {
        Q_OBJECT
        Q_DECLARE_PRIVATE(MessagingMenu::Application)
        private:
            ApplicationPrivate* d_ptr;
        public:
            class SourcePrivate;
            class Source
            {
                Q_DECLARE_PRIVATE(MessagingMenu::Application::Source)
                friend class Application;
                friend class ApplicationPrivate;
                private:
                    Source() = delete;
                    Source(const Source&) = delete;
                    Source& operator=(const Source&) = delete;
					Source(Application *application, const QString &id, const QString &label, const QString &icon = QString());
                public:
                    bool hasCount() const;
                    bool hasTime() const;
                    bool hasStr() const;

                    quint32 getCount() const;
                    QDateTime getTime() const;
                    const QString& getString() const;
                    const QString& getLabel() const;
					const QString& getIcon() const;

                    void setCount(quint32 count);
                    void setTime(const QDateTime &time);
                    void setString(const QString &str);
                    void setLabel(const QString &label);
					void setIcon(const QString &icon);
                    void setAttention(bool draw = true);
                    bool isPersistent() const;
                    void setPersistent(bool persistent = true);
                private:
                    void sendAll();
                    void setPosition(qint32 position);
                    qint32 getPosition () const;
					const QString& getId() const;
                private:
                    SourcePrivate *d_ptr;
            };

            Application(const QString &desktopId, QObject *parent = NULL);
            virtual ~Application();

			Source& createSourceTime(const QString &id, const QString &label, const QString &icon = QString(), const QDateTime &time = QDateTime::currentDateTime(), bool persistent = false, qint32 position = -1);
			Source& createSourceCount(const QString &id, const QString &label, const QString &icon = QString(), quint32 count = 0, bool persistent = false, qint32 position = -1);
			Source& createSourceString(const QString &id, const QString &label, const QString &icon = QString(), const QString& str = QString(), bool persistent = false, qint32 position = -1);
            void registerApp();
            void unregisterApp();
            void setStatus(Status status);
            void removeSource(Source& source);
            bool hasSource(const Source& source);
			bool hasSourceById(const QString& id);
        Q_SIGNALS:
            void sourceActivated(MessagingMenu::Application::Source&);
            void statusChanged(MessagingMenu::Status);
    };
}

#endif /* end of include guard: MESSAGING_MENU_30GOW558 */

