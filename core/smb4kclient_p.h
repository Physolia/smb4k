/*
    Private classes for the SMB client
    -------------------
    begin                : So Oct 21 2018
    SPDX-FileCopyrightText: 2018-2021 Alexander Reinholdt
    email                : alexander.reinholdt@kdemail.net
*/

/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   This program is distributed in the hope that it will be useful, but   *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     *
 *   General Public License for more details.                              *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc., 51 Franklin Street, Suite 500, Boston,*
 *   MA 02110-1335, USA                                                    *
 ***************************************************************************/

#ifndef SMB4KCLIENT_P_H
#define SMB4KCLIENT_P_H

// application specific includes
#include "smb4kclient.h"
#include "smb4kfile.h"
#include "smb4kglobal.h"
#include "smb4khost.h"
#include "smb4kshare.h"
#include "smb4kworkgroup.h"

// Samba includes
#include <libsmbclient.h>

// Qt includes
#include <QDialog>
#include <QHostAddress>
#include <QListWidgetItem>
#include <QTimer>
#include <QUrl>

// KDE includes
#include <KCoreAddons/KJob>
#include <KDNSSD/DNSSD/RemoteService>
#include <KDNSSD/DNSSD/ServiceBrowser>
#include <KIOCore/KFileItem>

#ifdef USE_WS_DISCOVERY
#include <WSDiscoveryClient>
#endif

class Smb4KClientBaseJob : public KJob
{
    Q_OBJECT

public:
    /**
     * Constructor
     */
    explicit Smb4KClientBaseJob(QObject *parent = nullptr);

    /**
     * Destructor
     */
    ~Smb4KClientBaseJob();

    /**
     * Set the process
     */
    void setProcess(Smb4KGlobal::Process process);

    /**
     * Return the process
     */
    Smb4KGlobal::Process process() const;

    /**
     * Set the basic network item
     */
    void setNetworkItem(NetworkItemPtr networkItem);

    /**
     * Return the basic network item
     */
    NetworkItemPtr networkItem() const;

    /**
     * The list of workgroups that was discovered.
     */
    QList<WorkgroupPtr> workgroups();

    /**
     * The list of hosts that was discovered.
     */
    QList<HostPtr> hosts();

    /**
     * The list shares that was discovered.
     */
    QList<SharePtr> shares();

    /**
     * The ist of files and directories that were discovered.
     */
    QList<FilePtr> files();

    /**
     * Error enumeration
     *
     * @enum ClientError         The client failed
     * @enum AccessDeniedError   Permission denied
     * @enum FileAccessError     The file could not be read
     * @enum PrintFileError      The file could not be printed
     */
    enum { ClientError = UserDefinedError, AccessDeniedError, FileAccessError, PrintFileError };

protected:
    Smb4KGlobal::Process *pProcess;
    NetworkItemPtr *pNetworkItem;
    QList<WorkgroupPtr> *pWorkgroups;
    QList<HostPtr> *pHosts;
    QList<SharePtr> *pShares;
    QList<FilePtr> *pFiles;
    QHostAddress lookupIpAddress(const QString &name);

private:
    Smb4KGlobal::Process m_process;
    NetworkItemPtr m_networkItem;
    QList<WorkgroupPtr> m_workgroups;
    QList<HostPtr> m_hosts;
    QList<SharePtr> m_shares;
    QList<FilePtr> m_files;
};

class Smb4KClientJob : public Smb4KClientBaseJob
{
    Q_OBJECT

public:
    /**
     * Constructor
     */
    explicit Smb4KClientJob(QObject *parent = nullptr);

    /**
     * Destructor
     */
    ~Smb4KClientJob();

    /**
     * Starts the job.
     */
    void start() override;

    /**
     * Set the file that is to be printed
     */
    void setPrintFileItem(const KFileItem &item);

    /**
     * Get the file that is to be printed
     */
    KFileItem printFileItem() const;

    /**
     * Set the number of copies that are to be printed
     */
    void setPrintCopies(int copies);

    /**
     * Get the number of copies that are to be printed
     */
    int printCopies() const;

    /**
     * The authentication function for libsmbclient
     */
    void get_auth_data_fn(const char *server,
                          const char *share,
                          char *workgroup,
                          int maxLenWorkgroup,
                          char *username,
                          int maxLenUsername,
                          char *password,
                          int maxLenPassword);

protected Q_SLOTS:
    void slotStartJob();

private:
    void initClientLibrary();
    void doLookups();
    void doPrinting();
    SMBCCTX *m_context;
    KFileItem m_fileItem;
    int m_copies;
};

class Smb4KDnsDiscoveryJob : public Smb4KClientBaseJob
{
    Q_OBJECT

public:
    /**
     * Constructor
     */
    explicit Smb4KDnsDiscoveryJob(QObject *parent = nullptr);

    /**
     * Destructor
     */
    ~Smb4KDnsDiscoveryJob();

    /**
     * Start the job
     */
    void start() override;

protected Q_SLOTS:
    void slotStartJob();
    void slotServiceAdded(KDNSSD::RemoteService::Ptr service);
    void slotFinished();

private:
    KDNSSD::ServiceBrowser *m_serviceBrowser;
};

#ifdef USE_WS_DISCOVERY
class Smb4KWsDiscoveryJob : public Smb4KClientBaseJob
{
    Q_OBJECT

public:
    /**
     * Constructor
     */
    explicit Smb4KWsDiscoveryJob(QObject *parent = nullptr);

    /**
     * Destructor
     */
    ~Smb4KWsDiscoveryJob();

    /**
     * Start the job
     */
    void start() override;

protected Q_SLOTS:
    void slotStartJob();
    void slotProbeMatchReceived(const WSDiscoveryTargetService &service);
    void slotResolveMatchReceived(const WSDiscoveryTargetService &service);
    void slotDiscoveryFinished();

private:
    WSDiscoveryClient *m_discoveryClient;
    QTimer *m_timer;
};
#endif

class Smb4KPreviewDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * Constructor
     */
    Smb4KPreviewDialog(const SharePtr &share, QWidget *parent = 0);

    /**
     * Destructor
     */
    ~Smb4KPreviewDialog();

    /**
     * Returns the share that is previewed
     *
     * @returns the share
     */
    SharePtr share() const;

Q_SIGNALS:
    /**
     * Request a preview
     */
    void requestPreview(NetworkItemPtr item);

    /**
     * Emitted when the dialog is about to close.
     */
    void aboutToClose(Smb4KPreviewDialog *dialog);

    /**
     * Emitted when the process should be aborted
     */
    void requestAbort();

protected Q_SLOTS:
    /**
     * Do last things before the dialog is closed
     */
    void slotClosingDialog();

    /**
     * Reload
     */
    void slotReloadActionTriggered();

    /**
     * Go up
     */
    void slotUpActionTriggered();

    /**
     * An URL was activated in the combo box
     *
     * @param url         The activated URL
     */
    void slotUrlActivated(const QUrl &url);

    /**
     * Process the selected item
     *
     * @param item        The activated item
     */
    void slotItemActivated(QListWidgetItem *item);

    /**
     * Initialize the preview
     */
    void slotInitializePreview();

    /**
     * Get the preview results and process them
     *
     * @param list        The list of the preview results
     */
    void slotPreviewResults(const QList<FilePtr> &list);

    /**
     * Change the reload/abort dual action
     */
    void slotAboutToStart(const NetworkItemPtr &item, int type);

    /**
     * Change the reload/abort dual action back
     */
    void slotFinished(const NetworkItemPtr &item, int type);

private:
    SharePtr m_share;
    NetworkItemPtr m_currentItem;
    QList<FilePtr> m_listing;
};

class Smb4KPrintDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * Constructor
     */
    Smb4KPrintDialog(const SharePtr &share, QWidget *parent = 0);

    /**
     * Destructor
     */
    ~Smb4KPrintDialog();

    /**
     * Returns the share that is printed to
     *
     * @returns the share
     */
    SharePtr share() const;

    /**
     * Returns the file item that is to be printed
     *
     * @returns the file
     */
    KFileItem fileItem() const;

Q_SIGNALS:
    /**
     * Emitted when a file is to be printed
     */
    void printFile(const SharePtr &printer, const KFileItem &file, int copies);

    /**
     * Emitted when the dialog is about to close
     */
    void aboutToClose(Smb4KPrintDialog *dialog);

protected Q_SLOTS:
    /**
     * Print button was clicked
     */
    void slotPrintButtonClicked();

    /**
     * Cancel button clicked
     */
    void slotCancelButtonClicked();

    /**
     * Called when the URL was changed
     */
    void slotUrlChanged();

private:
    SharePtr m_share;
    KFileItem m_fileItem;
};

class Smb4KClientPrivate
{
public:
    QList<Smb4KPreviewDialog *> previewDialogs;
    QList<Smb4KPrintDialog *> printDialogs;
    QList<WorkgroupPtr> tempWorkgroupList;
    QList<HostPtr> tempHostList;
};

class Smb4KClientStatic
{
public:
    Smb4KClient instance;
};

#endif
