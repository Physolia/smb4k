/*
    This class handles the homes shares

    SPDX-FileCopyrightText: 2006-2023 Alexander Reinholdt <alexander.reinholdt@kdemail.net>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef SMB4KHOMESSHARESHANDLER_H
#define SMB4KHOMESSHARESHANDLER_H

// application specific includes
#include "smb4kglobal.h"

// Qt includes
#include <QDialog>
#include <QList>
#include <QObject>
#include <QScopedPointer>
#include <QString>
#include <QStringList>

// forward declarations
class Smb4KAuthInfo;
class Smb4KHomesUsers;
class Smb4KHomesSharesHandlerPrivate;
class Smb4KProfileManager;

/**
 * This class belongs to the core of Smb4K and takes care of the
 * user names that are/were defined for a certain 'homes' share.
 *
 * @author Alexander Reinholdt <alexander.reinholdt@kdemail.net>
 */

class Q_DECL_EXPORT Smb4KHomesSharesHandler : public QObject
{
    Q_OBJECT

    friend class Smb4KHomesSharesHandlerPrivate;

public:
    /**
     * The constructor
     */
    explicit Smb4KHomesSharesHandler(QObject *parent = nullptr);

    /**
     * The destructor
     */
    ~Smb4KHomesSharesHandler();

    /**
     * Returns a static pointer to this class.
     */
    static Smb4KHomesSharesHandler *self();

    /**
     * This function will open a dialog where the user has to define a
     * user name to access a 'homes' share. You also can define if the
     * user name should be overwritten in case one has already been set
     * (default is TRUE).
     *
     * In case that a new username is set by this function, the password
     * is cleared.
     *
     * @param share       The share that is representing the homes share
     *
     * @param overwrite   Overwrite user name or not
     *
     * @returns TRUE if user has been chosen and FALSE otherwise.
     */
    SMB4K_DEPRECATED bool specifyUser(const SharePtr &share, bool overwrite = true);

    /**
     * Return the list of users defined for a certain homes share.
     *
     * @param share         The share
     *
     * @returns a list of users for the homes share
     */
    QStringList homesUsers(const SharePtr &share);

    /**
     * Add users to a homes share.
     *
     * @param share         The share
     *
     * @param userList      The list of users for this share
     */
    void addHomesUsers(const SharePtr &share, const QStringList &userList);

protected Q_SLOTS:
    /**
     * Called when a profile was removed
     *
     * @param name          The name of the profile
     */
    void slotProfileRemoved(const QString &name);

    /**
     * Called when a profile was migrated
     *
     * @param oldName       The old profile name
     * @param newName       The new profile name
     */
    void slotProfileMigrated(const QString &oldName, const QString &newName);

private:
    /**
     * Load the host and user names into a map.
     */
    void readUserNames();

    /**
     * This function writes the homes user entries to the disk.
     */
    void writeUserNames();

    /**
     * Pointer to the Smb4KHomesSharesHandlerPrivate class
     */
    const QScopedPointer<Smb4KHomesSharesHandlerPrivate> d;
};

#endif
