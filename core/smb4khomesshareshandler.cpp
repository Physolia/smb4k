/*
    This class handles the homes shares

    SPDX-FileCopyrightText: 2006-2023 Alexander Reinholdt <alexander.reinholdt@kdemail.net>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

// application specific includes
#include "smb4khomesshareshandler.h"
#include "smb4kauthinfo.h"
#include "smb4khomesshareshandler_p.h"
#include "smb4knotification.h"
#include "smb4kprofilemanager.h"
#include "smb4ksettings.h"
#include "smb4kshare.h"

// Qt includes
#include <QApplication>
#include <QFile>
#include <QPointer>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

// KDE includes
#include <KLocalizedString>

Q_GLOBAL_STATIC(Smb4KHomesSharesHandlerStatic, p);

Smb4KHomesSharesHandler::Smb4KHomesSharesHandler(QObject *parent)
    : QObject(parent)
    , d(new Smb4KHomesSharesHandlerPrivate)
{
    // First we need the directory.
    QString path = dataLocation();

    QDir dir;

    if (!dir.exists(path)) {
        dir.mkpath(path);
    }

    readUserNames();
}

Smb4KHomesSharesHandler::~Smb4KHomesSharesHandler()
{
    while (!d->homesUsers.isEmpty()) {
        delete d->homesUsers.takeFirst();
    }
}

Smb4KHomesSharesHandler *Smb4KHomesSharesHandler::self()
{
    return &p->instance;
}

bool Smb4KHomesSharesHandler::specifyUser(const SharePtr &share, bool overwrite)
{
    Q_ASSERT(share);
    bool success = false;

    // Avoid that the dialog is opened although the homes
    // user name has already been defined.
    if (share->isHomesShare() && (share->homeUrl().isEmpty() || overwrite)) {
        QStringList users = homesUsers(share);

        QPointer<Smb4KHomesUserDialog> dlg = new Smb4KHomesUserDialog(share, QApplication::activeWindow());
        dlg->setUserNames(users);

        if (dlg->exec() == QDialog::Accepted) {
            QString userName = dlg->userName();
            users = dlg->userNames();
            addHomesUsers(share, users);

            if (!userName.isEmpty()) {
                // If the login names do not match, clear the password.
                if (!share->userName().isEmpty() && QString::compare(share->userName(), userName) != 0) {
                    share->setPassword(QString());
                }

                // Set the login name.
                share->setUserName(userName);
                success = true;
            }

            writeUserNames();
        }

        delete dlg;
    } else {
        // The user name has already been set.
        success = true;
    }

    return success;
}

QStringList Smb4KHomesSharesHandler::homesUsers(const SharePtr &share)
{
    Q_ASSERT(share);

    QStringList userList;

    if (!d->homesUsers.isEmpty()) {
        for (const Smb4KHomesUsers *users : qAsConst(d->homesUsers)) {
            if (users->profile() == Smb4KSettings::activeProfile()
                && QString::compare(share->url().toString(QUrl::RemoveUserInfo | QUrl::RemovePort),
                                    users->url().toString(QUrl::RemoveUserInfo | QUrl::RemovePort))
                    == 0) {
                userList = users->userList();
                break;
            }
        }
    }

    return userList;
}

void Smb4KHomesSharesHandler::addHomesUsers(const SharePtr &share, const QStringList &userList)
{
    Q_ASSERT(share);

    bool found = false;

    if (!d->homesUsers.isEmpty()) {
        QMutableListIterator<Smb4KHomesUsers *> it(d->homesUsers);

        while (it.hasNext()) {
            Smb4KHomesUsers *users = it.next();

            if (users->profile() == Smb4KSettings::activeProfile()
                && QString::compare(share->url().toString(QUrl::RemoveUserInfo | QUrl::RemovePort),
                                    users->url().toString(QUrl::RemoveUserInfo | QUrl::RemovePort))
                    == 0) {
                users->setUserList(userList);
                found = true;
                break;
            }
        }
    }

    if (!found) {
        Smb4KHomesUsers *users = new Smb4KHomesUsers(share, userList);
        users->setProfile(Smb4KProfileManager::self()->activeProfile());
        d->homesUsers << users;
    }

    writeUserNames();
}

void Smb4KHomesSharesHandler::readUserNames()
{
    // Locate the XML file.
    QFile xmlFile(dataLocation() + QDir::separator() + QStringLiteral("homes_shares.xml"));

    if (xmlFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QXmlStreamReader xmlReader(&xmlFile);

        QString versionString;

        while (!xmlReader.atEnd()) {
            xmlReader.readNext();

            if (xmlReader.isStartElement()) {
                if (xmlReader.name() == QStringLiteral("homes_shares")
                    && (xmlReader.attributes().value(QStringLiteral("version")) != QStringLiteral("1.0")
                        || xmlReader.attributes().value(QStringLiteral("version")) != QStringLiteral("2.0"))) {
                    xmlReader.raiseError(i18n("The format of %1 is not supported.", xmlFile.fileName()));
                    break;
                } else {
                    // NOTE: Forbackward compatibility. Remove with version >> 4.0
                    if (xmlReader.name() == QStringLiteral("homes_shares")) {
                        versionString = xmlReader.attributes().value(QStringLiteral("version")).toString();
                    }

                    if (versionString == QStringLiteral("1.0")) {
                        if (xmlReader.name() == QStringLiteral("homes")) {
                            QString profile = xmlReader.attributes().value(QStringLiteral("profile")).toString();

                            Smb4KHomesUsers *users = new Smb4KHomesUsers();
                            users->setProfile(profile);

                            QUrl url;
                            url.setScheme(QStringLiteral("smb"));
                            url.setPath(xmlReader.name().toString());

                            while (!(xmlReader.isEndElement() && xmlReader.name() == QStringLiteral("homes"))) {
                                xmlReader.readNext();

                                if (xmlReader.isStartElement()) {
                                    if (xmlReader.name() == QStringLiteral("host")) {
                                        url.setHost(xmlReader.readElementText());
                                    } else if (xmlReader.name() == QStringLiteral("workgroup")) {
                                        users->setWorkgroupName(xmlReader.readElementText());
                                    } else if (xmlReader.name() == QStringLiteral("ip")) {
                                        users->setHostIP(xmlReader.readElementText());
                                    } else if (xmlReader.name() == QStringLiteral("users")) {
                                        QStringList u;

                                        while (!(xmlReader.isEndElement() && xmlReader.name() == QStringLiteral("users"))) {
                                            xmlReader.readNext();

                                            if (xmlReader.isStartElement() && xmlReader.name() == QStringLiteral("user")) {
                                                u << xmlReader.readElementText();
                                            }
                                        }

                                        users->setUserList(u);
                                    }
                                }
                            }

                            users->setUrl(url);

                            d->homesUsers << users;
                        }
                    } else if (versionString == QStringLiteral("2.0")) {
                        if (xmlReader.name().endsWith(QStringLiteral("homes"))) {
                            QUrl url(xmlReader.name().toString());
                            QString profile = xmlReader.attributes().value(QStringLiteral("profile")).toString();

                            Smb4KHomesUsers *users = new Smb4KHomesUsers();
                            users->setProfile(profile);
                            users->setUrl(url);

                            while (!(xmlReader.isEndElement() && xmlReader.name() == QStringLiteral("homes"))) {
                                xmlReader.readNext();

                                if (xmlReader.isStartElement()) {
                                    if (xmlReader.name() == QStringLiteral("workgroup")) {
                                        users->setWorkgroupName(xmlReader.readElementText());
                                    } else if (xmlReader.name() == QStringLiteral("ip")) {
                                        users->setHostIP(xmlReader.readElementText());
                                    } else if (xmlReader.name() == QStringLiteral("users")) {
                                        QStringList u;

                                        while (!(xmlReader.isEndElement() && xmlReader.name() == QStringLiteral("users"))) {
                                            xmlReader.readNext();

                                            if (xmlReader.isStartElement() && xmlReader.name() == QStringLiteral("user")) {
                                                u << xmlReader.readElementText();
                                            }
                                        }

                                        users->setUserList(u);
                                    }
                                }
                            }

                            d->homesUsers << users;
                        }
                    }
                }
            }
        }

        xmlFile.close();

        if (xmlReader.hasError()) {
            Smb4KNotification::readingFileFailed(xmlFile, xmlReader.errorString());
        }
    } else {
        if (xmlFile.exists()) {
            Smb4KNotification::openingFileFailed(xmlFile);
        }
    }
}

void Smb4KHomesSharesHandler::writeUserNames()
{
    // FIXME: Use IP address and workgroup at all? We really only need the URL.
    QFile xmlFile(dataLocation() + QDir::separator() + QStringLiteral("homes_shares.xml"));

    if (!d->homesUsers.isEmpty()) {
        if (xmlFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QXmlStreamWriter xmlWriter(&xmlFile);
            xmlWriter.setAutoFormatting(true);
            xmlWriter.writeStartDocument();
            xmlWriter.writeStartElement(QStringLiteral("homes_shares"));
            xmlWriter.writeAttribute(QStringLiteral("version"), QStringLiteral("2.0"));

            for (Smb4KHomesUsers *users : qAsConst(d->homesUsers)) {
                xmlWriter.writeStartElement(users->url().toString(QUrl::StripTrailingSlash));
                xmlWriter.writeAttribute(QStringLiteral("profile"), users->profile());
                xmlWriter.writeTextElement(QStringLiteral("workgroup"), users->workgroupName());
                xmlWriter.writeTextElement(QStringLiteral("ip"), users->hostIP());
                xmlWriter.writeStartElement(QStringLiteral("users"));

                QStringList userList = users->userList();

                for (const QString &user : qAsConst(userList)) {
                    xmlWriter.writeTextElement(QStringLiteral("user"), user);
                }

                xmlWriter.writeEndElement();
                xmlWriter.writeEndElement();
            }

            xmlWriter.writeEndDocument();
            xmlFile.close();
        } else {
            Smb4KNotification::openingFileFailed(xmlFile);
        }
    } else {
        xmlFile.remove();
    }
}

/////////////////////////////////////////////////////////////////////////////
// SLOT IMPLEMENTATIONS
/////////////////////////////////////////////////////////////////////////////

void Smb4KHomesSharesHandler::slotProfileRemoved(const QString &name)
{
    QMutableListIterator<Smb4KHomesUsers *> it(d->homesUsers);

    while (it.hasNext()) {
        Smb4KHomesUsers *user = it.next();

        if (name == user->profile()) {
            it.remove();
        }
    }

    writeUserNames();
}

void Smb4KHomesSharesHandler::slotProfileMigrated(const QString &oldName, const QString &newName)
{
    for (int i = 0; i < d->homesUsers.size(); i++) {
        if (oldName == d->homesUsers.at(i)->profile()) {
            d->homesUsers[i]->setProfile(newName);
        }
    }

    writeUserNames();
}
