/*
    Provides an interface to the computer's hardware

    SPDX-FileCopyrightText: 2015-2023 Alexander Reinholdt <alexander.reinholdt@kdemail.net>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef SMB4KHARDWAREINTERFACE_H
#define SMB4KHARDWAREINTERFACE_H

// application specific includes
#include "smb4kglobal.h"

// Qt includes
#include <QObject>
#include <QScopedPointer>
#include <QUrl>

class Smb4KHardwareInterfacePrivate;

/**
 * This class provides an interface to the computer's hardware.
 * @author Alexander Reinholdt <alexander.reinholdt@kdemail.net>
 * @since 2.0.0
 */

class Q_DECL_EXPORT Smb4KHardwareInterface : public QObject
{
    Q_OBJECT

    friend class Smb4KHardwareInterfacePrivate;

public:
    /**
     * The constructor
     */
    explicit Smb4KHardwareInterface(QObject *parent = nullptr);

    /**
     * The destructor
     */
    ~Smb4KHardwareInterface();

    /**
     * The static pointer to this class.
     * @returns a static pointer to this class
     */
    static Smb4KHardwareInterface *self();

    /**
     * This function returns TRUE if the system is online and FALSE otherwise.
     * @returns TRUE if the system is online.
     */
    bool isOnline() const;

    /**
     * Inhibit shutdown and sleep.
     */
    void inhibit();

    /**
     * Uninhibit shutdown and sleep.
     */
    void uninhibit();

protected:
    /**
     * Reimplemented from QObject to check for mounts and unmounts on operating
     * systems that are not fully supported by Solid, yet.
     */
    void timerEvent(QTimerEvent *e) override;

Q_SIGNALS:
    /**
     * This signal is emitted when a network share is added to the system
     */
    void networkShareAdded();

    /**
     * This signal is emitted when a network share is removed from the system
     */
    void networkShareRemoved();

    /**
     * This signal is emitted when the online state changed.
     */
    void onlineStateChanged(bool online);

protected Q_SLOTS:
    /**
     * This slot is called when a device was added to the system.
     * @param udi     the device UDI
     */
    void slotDeviceAdded(const QString &udi);

    /**
     * This slot is called when a device was removed from the system.
     * @param udi     the device UDI
     */
    void slotDeviceRemoved(const QString &udi);

private:
    /**
     * Check the online state and emit the @see onlineStateChanged() accordingly, if
     * @p emitSignal is set to TRUE.
     */
    void checkOnlineState(bool emitSignal = true);

    /**
     * Pointer to private class
     */
    QScopedPointer<Smb4KHardwareInterfacePrivate> d;
};

#endif
