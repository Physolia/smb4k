/*
    The main window of Smb4K

    SPDX-FileCopyrightText: 2008-2022 Alexander Reinholdt <alexander.reinholdt@kdemail.net>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

// application specific includes
#include "smb4kmainwindow.h"
#include "core/smb4kclient.h"
#include "core/smb4kfile.h"
#include "core/smb4kglobal.h"
#include "core/smb4khost.h"
#include "core/smb4kmounter.h"
#include "core/smb4ksettings.h"
#include "core/smb4kshare.h"
#include "core/smb4ksynchronizer.h"
#include "core/smb4kwalletmanager.h"
#include "core/smb4kworkgroup.h"
#include "smb4kbookmarkmenu.h"
#include "smb4knetworkbrowserdockwidget.h"
#include "smb4kprofilesmenu.h"
#include "smb4ksharesviewdockwidget.h"
#include "smb4ksystemtray.h"

// Qt includes
#include <QActionGroup>
#include <QApplication>
#include <QDockWidget>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QSize>
#include <QStatusBar>
#include <QString>
#include <QTabBar>
#include <QTimer>
#include <QVariantList>

// KDE includes
#include "kiconthemes_version.h"
#include <KConfigWidgets/KConfigDialog>
#include <KConfigWidgets/KStandardAction>
#include <KCoreAddons/KPluginFactory>
#include <KCoreAddons/KPluginLoader>
#include <KI18n/KLocalizedString>
#include <KIconThemes/KIconLoader>
#include <KWidgetsAddons/KMessageBox>
#include <KXmlGui/KXMLGUIFactory>

using namespace Smb4KGlobal;
using namespace KParts;

Smb4KMainWindow::Smb4KMainWindow()
    : KXmlGuiWindow()
    , m_system_tray_widget(nullptr)
{
    //
    // The widget (embedded into the dock widgets) that has the focus
    //
    m_focusWidget = nullptr;

    //
    // Set up main window
    //
    setStandardToolBarMenuEnabled(true);
    createStandardStatusBarAction();
    setDockNestingEnabled(true);
    setupActions();
    setupGUI(Default, QStringLiteral("smb4k_shell.rc"));
    setupView();
    setupMenuBar();
    setupStatusBar();
    setupSystemTrayWidget();

    //
    // Set the tab orientation
    //
    switch (Smb4KSettings::mainWindowTabOrientation()) {
    case Smb4KSettings::EnumMainWindowTabOrientation::Top: {
        setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::North);
        break;
    }
    case Smb4KSettings::EnumMainWindowTabOrientation::Bottom: {
        setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::South);
        break;
    }
    case Smb4KSettings::EnumMainWindowTabOrientation::Left: {
        setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::West);
        break;
    }
    case Smb4KSettings::EnumMainWindowTabOrientation::Right: {
        setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::East);
        break;
    }
    default: {
        break;
    }
    }

    //
    // Apply the main window settings
    //
    setAutoSaveSettings(KConfigGroup(Smb4KSettings::self()->config(), "MainWindow"), true);

    //
    // Save the setting no matter how the application is closed
    //
    connect(qApp, &QCoreApplication::aboutToQuit, this, [this]() {
        saveSettings();
    });
}

Smb4KMainWindow::~Smb4KMainWindow()
{
}

void Smb4KMainWindow::setupActions()
{
    //
    // Quit action
    //
    QAction *quit_action = KStandardAction::quit(this, SLOT(slotQuit()), actionCollection());
    actionCollection()->addAction(QStringLiteral("quit_action"), quit_action);

    //
    // Configure action
    //
    QAction *configure_action = KStandardAction::preferences(this, SLOT(slotConfigDialog()), actionCollection());
    actionCollection()->addAction(QStringLiteral("configure_action"), configure_action);

    //
    // Dock widgets action menu
    //
    KActionMenu *dock_widgets_menu = new KActionMenu(KDE::icon(QStringLiteral("tab-duplicate")), i18n("Dock Widgets"), actionCollection());
    actionCollection()->addAction(QStringLiteral("dock_widgets_menu"), dock_widgets_menu);

    m_dockWidgets = new QActionGroup(actionCollection());
    m_dockWidgets->setExclusive(false);

    //
    // Bookmarks menu and action
    //
    Smb4KBookmarkMenu *bookmarksMenu = new Smb4KBookmarkMenu(Smb4KBookmarkMenu::MainWindow, this);
    QAction *addBookmarkAction = new QAction(KDE::icon(QStringLiteral("bookmark-new")), i18n("Add &Bookmark"), actionCollection());
    addBookmarkAction->setEnabled(false);
    actionCollection()->addAction(QStringLiteral("bookmarks_menu"), bookmarksMenu);
    actionCollection()->addAction(QStringLiteral("bookmark_action"), addBookmarkAction);
    connect(addBookmarkAction, SIGNAL(triggered(bool)), this, SLOT(slotAddBookmarks()));
    connect(bookmarksMenu, SIGNAL(addBookmark()), this, SLOT(slotAddBookmarks()));

    //
    // Profiles menu
    //
    Smb4KProfilesMenu *profiles = new Smb4KProfilesMenu(this);
    actionCollection()->addAction(QStringLiteral("profiles_menu"), profiles);
}

void Smb4KMainWindow::setupStatusBar()
{
    // Set up the progress bar.
    m_progress_bar = new QProgressBar(statusBar());
    m_progress_bar->setFixedWidth(100);
    m_progress_bar->setMaximum(0);
    m_progress_bar->setMinimum(0);
    m_progress_bar->setFixedHeight(statusBar()->fontMetrics().height());
    m_progress_bar->setAlignment(Qt::AlignCenter);
    m_progress_bar->setVisible(false);

    // Set the icon on the right side that represents the initial
    // state of the wallet manager.
    m_pass_icon = new QLabel(statusBar());
    m_pass_icon->setContentsMargins(0, 0, 0, 0);
    m_pass_icon->setAlignment(Qt::AlignCenter);

    // The feedback icon.
    m_feedback_icon = new QLabel(statusBar());
    m_feedback_icon->setContentsMargins(0, 0, 0, 0);
    m_feedback_icon->setAlignment(Qt::AlignCenter);

    statusBar()->addPermanentWidget(m_progress_bar);
    statusBar()->addPermanentWidget(m_feedback_icon);
    statusBar()->addPermanentWidget(m_pass_icon);

    slotWalletManagerInitialized();
    setupMountIndicator();

    //
    // Connections
    //
    connect(Smb4KClient::self(), SIGNAL(aboutToStart(NetworkItemPtr, int)), this, SLOT(slotClientAboutToStart(NetworkItemPtr, int)));
    connect(Smb4KClient::self(), SIGNAL(finished(NetworkItemPtr, int)), this, SLOT(slotClientFinished(NetworkItemPtr, int)));

    connect(Smb4KWalletManager::self(), SIGNAL(initialized()), this, SLOT(slotWalletManagerInitialized()));

    connect(Smb4KMounter::self(), SIGNAL(mounted(SharePtr)), this, SLOT(slotVisualMountFeedback(SharePtr)));
    connect(Smb4KMounter::self(), SIGNAL(unmounted(SharePtr)), this, SLOT(slotVisualUnmountFeedback(SharePtr)));
    connect(Smb4KMounter::self(), SIGNAL(aboutToStart(int)), this, SLOT(slotMounterAboutToStart(int)));
    connect(Smb4KMounter::self(), SIGNAL(finished(int)), this, SLOT(slotMounterFinished(int)));

    connect(Smb4KSynchronizer::self(), SIGNAL(aboutToStart(QString)), this, SLOT(slotSynchronizerAboutToStart(QString)));
    connect(Smb4KSynchronizer::self(), SIGNAL(finished(QString)), this, SLOT(slotSynchronizerFinished(QString)));
}

void Smb4KMainWindow::setupView()
{
    //
    // We do not set a central widget, because it causes "problems"
    // with the dock widgets. We have the nested dock widget property
    // set to true, so we can arrange the dock widgets as we like,
    // nonetheless.
    //

    //
    // Network browser dock widget
    //
    Smb4KNetworkBrowserDockWidget *networkBrowserDock = new Smb4KNetworkBrowserDockWidget(i18n("Network Neighborhood"), this);
    networkBrowserDock->setObjectName(QStringLiteral("NetworkBrowserDockWidget"));
    networkBrowserDock->setAllowedAreas(Qt::LeftDockWidgetArea);

    // Install event filter
    networkBrowserDock->widget()->installEventFilter(this);

    // Connections
    connect(networkBrowserDock, SIGNAL(visibilityChanged(bool)), SLOT(slotNetworkBrowserVisibilityChanged(bool)));

    // Add dock widget
    addDockWidget(Qt::LeftDockWidgetArea, networkBrowserDock);

    // Insert the toggle view mode action to the action group.
    m_dockWidgets->addAction(networkBrowserDock->toggleViewAction());
    static_cast<KActionMenu *>(actionCollection()->action(QStringLiteral("dock_widgets_menu")))->addAction(networkBrowserDock->toggleViewAction());

    // Insert the Network menu
    plugActionList(QStringLiteral("network_menu"), networkBrowserDock->actionCollection()->actions());

    //
    // Shares view dock widget
    //
    Smb4KSharesViewDockWidget *sharesViewDock = new Smb4KSharesViewDockWidget(i18n("Mounted Shares"), this);
    sharesViewDock->setObjectName(QStringLiteral("SharesViewDockWidget"));
    sharesViewDock->setAllowedAreas(Qt::LeftDockWidgetArea);

    // Install event filter
    sharesViewDock->widget()->installEventFilter(this);

    // Connections
    connect(sharesViewDock, SIGNAL(visibilityChanged(bool)), this, SLOT(slotSharesViewVisibilityChanged(bool)));

    // Add dock widget
    addDockWidget(Qt::LeftDockWidgetArea, sharesViewDock);

    // Insert the toggle view mode action to the action group.
    m_dockWidgets->addAction(sharesViewDock->toggleViewAction());
    static_cast<KActionMenu *>(actionCollection()->action(QStringLiteral("dock_widgets_menu")))->addAction(sharesViewDock->toggleViewAction());

    // Insert the Shares menu
    plugActionList(QStringLiteral("shares_menu"), sharesViewDock->actionCollection()->actions());

    //
    // Initial main window look
    //
    KConfigGroup configGroup(Smb4KSettings::self()->config(), "MainWindow");

    if (!configGroup.hasKey(QStringLiteral("FirstStartup"))) {
        QList<QDockWidget *> docks = findChildren<QDockWidget *>();

        for (int i = 1; i < docks.size(); ++i) {
            tabifyDockWidget(docks.at(i - 1), docks.at(i));
        }
        
        configGroup.writeEntry(QStringLiteral("FirstStartup"), true);
    }
}

void Smb4KMainWindow::setupMenuBar()
{
    // Get the "Bookmarks" menu
    QList<QAction *> actions = menuBar()->actions();
    QListIterator<QAction *> it(actions);

    while (it.hasNext()) {
        QAction *action = it.next();

        if (action->objectName() == QStringLiteral("bookmarks")) {
            Smb4KBookmarkMenu *menu = static_cast<Smb4KBookmarkMenu *>(actionCollection()->action(QStringLiteral("bookmarks_menu")));
            action->setMenu(menu->menu());
            break;
        } else {
            continue;
        }
    }
}

void Smb4KMainWindow::setupSystemTrayWidget()
{
    if (!m_system_tray_widget) {
        m_system_tray_widget = new Smb4KSystemTray(this);
    }

    connect(m_system_tray_widget, SIGNAL(settingsChanged(QString)), this, SLOT(slotSettingsChanged(QString)));
}

void Smb4KMainWindow::loadSettings()
{
    //
    // Main window
    //
    switch (Smb4KSettings::mainWindowTabOrientation()) {
    case Smb4KSettings::EnumMainWindowTabOrientation::Top: {
        setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::North);
        break;
    }
    case Smb4KSettings::EnumMainWindowTabOrientation::Bottom: {
        setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::South);
        break;
    }
    case Smb4KSettings::EnumMainWindowTabOrientation::Left: {
        setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::West);
        break;
    }
    case Smb4KSettings::EnumMainWindowTabOrientation::Right: {
        setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::East);
        break;
    }
    default: {
        break;
    }
    }

    //
    // Let the network browser load its settings
    //
    Smb4KNetworkBrowserDockWidget *networkBrowserDock = findChild<Smb4KNetworkBrowserDockWidget *>();

    if (networkBrowserDock) {
        networkBrowserDock->loadSettings();
    }

    //
    // Let the shares view load its settings
    //
    Smb4KSharesViewDockWidget *sharesViewDock = findChild<Smb4KSharesViewDockWidget *>();

    if (sharesViewDock) {
        sharesViewDock->loadSettings();
    }

    //
    // Reload the list of bookmarks
    //
    Smb4KBookmarkMenu *bookmarkMenu = findChild<Smb4KBookmarkMenu *>();

    if (bookmarkMenu) {
        bookmarkMenu->refreshMenu();
    }

    // Check the state of the password handler and the wallet settings and
    // set the pixmap in the status bar accordingly.
    slotWalletManagerInitialized();

    // Set up the mount indicator icon.
    setupMountIndicator();
}

void Smb4KMainWindow::saveSettings()
{
    //
    // Save the settings of the network browser
    //
    Smb4KNetworkBrowserDockWidget *networkBrowserDock = findChild<Smb4KNetworkBrowserDockWidget *>();

    if (networkBrowserDock) {
        networkBrowserDock->saveSettings();
    }

    //
    // Let the shares view load its settings
    //
    Smb4KSharesViewDockWidget *sharesViewDock = findChild<Smb4KSharesViewDockWidget *>();

    if (sharesViewDock) {
        sharesViewDock->saveSettings();
    }

    //
    // Save if the main window should be started docked.
    //
    Smb4KSettings::setStartMainWindowDocked(!isVisible());

    //
    // Save the settings
    //
    Smb4KSettings::self()->save();
}

bool Smb4KMainWindow::queryClose()
{
    if (!QApplication::closingDown() && !qApp->isSavingSession() && isVisible()) {
        // This part has been copied from JuK application.
        KMessageBox::information(this,
                                 i18n("<qt>Closing the main window will keep Smb4K running in the system tray.<br>"
                                      "Use <i>Quit</i> from the <i>File</i> menu to quit the application.</qt>"),
                                 i18n("Docking"),
                                 QStringLiteral("DockToSystemTrayInfo"));
        setVisible(false);
        return false;
    } else {
        saveSettings();
        return true;
    }
}

bool Smb4KMainWindow::eventFilter(QObject *obj, QEvent *e)
{
    switch (e->type()) {
    case QEvent::FocusIn: {
        //
        // Check if the widget that has the focus belongs to the network
        // browser widget
        //
        Smb4KNetworkBrowserDockWidget *networkBrowserDock = findChild<Smb4KNetworkBrowserDockWidget *>();

        if (networkBrowserDock) {
            QObjectList children = networkBrowserDock->children();

            for (QObject *object : children) {
                if (object == obj) {
                    m_focusWidget = networkBrowserDock;
                    setupDynamicActionList(networkBrowserDock);
                    break;
                }
            }
        }

        //
        // Check if the widget that has the focus belongs to the shares
        // view
        //
        Smb4KSharesViewDockWidget *sharesViewDock = findChild<Smb4KSharesViewDockWidget *>();

        if (sharesViewDock) {
            QObjectList children = sharesViewDock->children();

            for (QObject *object : children) {
                if (object == obj) {
                    m_focusWidget = sharesViewDock;
                    setupDynamicActionList(sharesViewDock);
                    break;
                }
            }
        }
        break;
    }
    default: {
        break;
    }
    }

    return KXmlGuiWindow::eventFilter(obj, e);
}

void Smb4KMainWindow::setupMountIndicator()
{
    QStringList overlays;

    if (mountedSharesList().size() == 0) {
        m_feedback_icon->setToolTip(i18n("There are currently no shares mounted."));
    } else {
        overlays << QStringLiteral("emblem-mounted");
        m_feedback_icon->setToolTip(i18np("There is currently %1 share mounted.", "There are currently %1 shares mounted.", mountedSharesList().size()));
    }

    m_feedback_icon->setPixmap(KIconLoader::global()->loadIcon(QStringLiteral("folder-network"), KIconLoader::Small, 0, KIconLoader::DefaultState, overlays));
}

void Smb4KMainWindow::setupDynamicActionList(QDockWidget *dock)
{
    if (dock) {
        //
        // Remove all connections to Smb4KMainWindow::slotEnableBookmarkAction() and
        // disable the bookmark action.
        //
        disconnect(this, SLOT(slotEnableBookmarkAction()));
        actionCollection()->action(QStringLiteral("bookmark_action"))->setEnabled(false);

        //
        // Get also the bookmark menu and disable the bookmark action
        // there, too.
        //
        Smb4KBookmarkMenu *bookmarkMenu = findChild<Smb4KBookmarkMenu *>();

        if (bookmarkMenu) {
            bookmarkMenu->setBookmarkActionEnabled(false);
        }

        //
        // Prepare the dynamic action list for the main window
        //
        QList<QAction *> dynamicList;
        QList<QAction *> actionsList;

        if (dock->objectName() == QStringLiteral("NetworkBrowserDockWidget")) {
            actionsList = static_cast<Smb4KNetworkBrowserDockWidget *>(dock)->actionCollection()->actions();
        } else if (dock->objectName() == QStringLiteral("SharesViewDockWidget")) {
            actionsList = static_cast<Smb4KSharesViewDockWidget *>(dock)->actionCollection()->actions();
        }

        for (QAction *action : qAsConst(actionsList)) {
            if (action->objectName() == QStringLiteral("bookmark_action")) {
                if (bookmarkMenu) {
                    bookmarkMenu->setBookmarkActionEnabled(action->isEnabled());
                    connect(action, SIGNAL(changed()), this, SLOT(slotEnableBookmarkAction()));
                    continue;
                }
            } else if (action->objectName() == QStringLiteral("filemanager_action")) {
                continue;
            } else if (action->objectName() == QStringLiteral("konsole_action")) {
                continue;
            } else if (action->objectName() == QStringLiteral("icon_view_action")) {
                continue;
            } else if (action->objectName() == QStringLiteral("list_view_action")) {
                continue;
            }

            dynamicList << action;
        }

        //
        // Remove old and insert new dynamic action list
        //
        unplugActionList(QStringLiteral("dynamic_list"));
        plugActionList(QStringLiteral("dynamic_list"), dynamicList);
    }
}

/////////////////////////////////////////////////////////////////////////////
// SLOT IMPLEMENTATIONS
/////////////////////////////////////////////////////////////////////////////

void Smb4KMainWindow::slotQuit()
{
    //
    // Save the settings. Work around queryClose() not being called. :(
    //
    saveSettings();

    //
    // Quit the application
    //
    qApp->quit();
}

void Smb4KMainWindow::slotConfigDialog()
{
    //
    // Check if the configuration dialog exists and try to show it.
    //
    if (KConfigDialog::exists(QStringLiteral("Smb4KConfigDialog"))) {
        KConfigDialog::showDialog(QStringLiteral("Smb4KConfigDialog"));
        return;
    }

    //
    // If the dialog does not exist, load and show it:
    //
    KPluginLoader loader(QStringLiteral("smb4kconfigdialog"));
    KPluginFactory *configFactory = loader.factory();

    if (configFactory) {
        KConfigDialog *dlg = configFactory->create<KConfigDialog>(this);

        if (dlg) {
            dlg->setObjectName(QStringLiteral("Smb4KConfigDialog"));
            connect(dlg, SIGNAL(settingsChanged(QString)), this, SLOT(slotSettingsChanged(QString)), Qt::UniqueConnection);
            connect(dlg, SIGNAL(settingsChanged(QString)), m_system_tray_widget, SLOT(slotSettingsChanged(QString)), Qt::UniqueConnection);
            dlg->show();
        }
    } else {
        KMessageBox::error(nullptr, loader.errorString());
        return;
    }
}

void Smb4KMainWindow::slotSettingsChanged(const QString &)
{
    loadSettings();
}

void Smb4KMainWindow::slotAddBookmarks()
{
    //
    // If we have a widget that has the focus, trigger its 'Add Bookmark'
    // action to add bookmarks.
    //
    if (m_focusWidget) {
        if (qstrcmp(m_focusWidget->metaObject()->className(), "Smb4KNetworkBrowserDockWidget") == 0) {
            Smb4KNetworkBrowserDockWidget *dockWidget = qobject_cast<Smb4KNetworkBrowserDockWidget *>(m_focusWidget);

            if (dockWidget) {
                QAction *action = dockWidget->actionCollection()->action(QStringLiteral("bookmark_action"));

                // Only trigger the action if it is enabled
                if (action && action->isEnabled()) {
                    action->trigger();
                }
            }
        } else if (qstrcmp(m_focusWidget->metaObject()->className(), "Smb4KSharesViewDockWidget") == 0) {
            Smb4KSharesViewDockWidget *dockWidget = qobject_cast<Smb4KSharesViewDockWidget *>(m_focusWidget);

            if (dockWidget) {
                QAction *action = dockWidget->actionCollection()->action(QStringLiteral("bookmark_action"));

                // Only trigger the action if it is enabled
                if (action && action->isEnabled()) {
                    action->trigger();
                }
            }
        }
    }
}

void Smb4KMainWindow::slotWalletManagerInitialized()
{
    if (Smb4KWalletManager::self()->useWalletSystem()) {
        if (KIconLoader::global()->hasIcon(QStringLiteral("kwalletmanager"))) {
            m_pass_icon->setPixmap(KIconLoader::global()->loadIcon(QStringLiteral("kwalletmanager"), KIconLoader::Small, 0, KIconLoader::DefaultState));
        } else {
            m_pass_icon->setPixmap(KIconLoader::global()->loadIcon(QStringLiteral("security-high"), KIconLoader::Small, 0, KIconLoader::DefaultState));
        }

        m_pass_icon->setToolTip(i18n("The wallet is used."));
    } else {
        m_pass_icon->setPixmap(KIconLoader::global()->loadIcon(QStringLiteral("dialog-password"), KIconLoader::Small, 0, KIconLoader::DefaultState));
        m_pass_icon->setToolTip(i18n("The password dialog is used."));
    }
}

void Smb4KMainWindow::slotClientAboutToStart(const NetworkItemPtr &item, int process)
{
    Q_ASSERT(item);

    switch (process) {
    case LookupDomains: {
        statusBar()->showMessage(i18n("Looking for workgroups and domains..."), 0);
        break;
    }
    case LookupDomainMembers: {
        WorkgroupPtr workgroup = item.staticCast<Smb4KWorkgroup>();
        statusBar()->showMessage(i18n("Looking for hosts in domain %1...", workgroup->workgroupName()), 0);
        break;
    }
    case LookupShares: {
        HostPtr host = item.staticCast<Smb4KHost>();
        statusBar()->showMessage(i18n("Looking for shares provided by host %1...", host->hostName()), 0);
        break;
    }
    case LookupFiles: {
        QString message;

        switch (item->type()) {
        case Share: {
            message = i18n("Looking for files and directories in %1...", item.staticCast<Smb4KShare>()->displayString());
            break;
        }
        case Directory: {
            FilePtr file = item.staticCast<Smb4KFile>();

            for (const SharePtr &s : sharesList()) {
                if (s->workgroupName() == file->workgroupName() && s->hostName() == file->hostName() && s->shareName() == file->shareName()) {
                    message = i18n("Looking for files and directories in %1...", s->displayString());
                    break;
                }
            }

            break;
        }
        default: {
            break;
        }
        }

        statusBar()->showMessage(message, 0);

        break;
    }
    case WakeUp: {
        statusBar()->showMessage(i18n("Waking up remote hosts..."), 0);
        break;
    }
    case PrintFile: {
        SharePtr share = item.staticCast<Smb4KShare>();
        statusBar()->showMessage(i18n("Sending file to printer %1...", share->displayString()), 0);
        break;
    }
    case NetworkSearch: {
        statusBar()->showMessage(i18n("Searching..."), 0);
        break;
    }
    default: {
        break;
    }
    }

    if (!m_progress_bar->isVisible()) {
        m_progress_bar->setVisible(true);
    }
}

void Smb4KMainWindow::slotClientFinished(const NetworkItemPtr & /*item*/, int /*process*/)
{
    if (!coreIsRunning()) {
        m_progress_bar->setVisible(false);
        m_progress_bar->reset();
        statusBar()->showMessage(i18n("Done."), 2000);
    }
}

void Smb4KMainWindow::slotMounterAboutToStart(int process)
{
    // Tell the user which action is performed by the mounter:
    // mounting, unmounting or waking up.
    switch (process) {
    case MountShare: {
        statusBar()->showMessage(i18n("Mounting..."), 0);
        break;
    }
    case UnmountShare: {
        statusBar()->showMessage(i18n("Unmounting..."), 0);
        break;
    }
    case WakeUp: {
        statusBar()->showMessage(i18n("Waking up host..."), 0);
        break;
    }
    default: {
        break;
    }
    }

    if (!m_progress_bar->isVisible()) {
        m_progress_bar->setVisible(true);
    }
}

void Smb4KMainWindow::slotMounterFinished(int /*process*/)
{
    QTimer::singleShot(250, this, [this]() {
        if (!coreIsRunning()) {
            m_progress_bar->setVisible(false);
            m_progress_bar->reset();
            statusBar()->showMessage(i18n("Done."), 2000);
        }
    });
}

void Smb4KMainWindow::slotVisualMountFeedback(const SharePtr &share)
{
    Q_ASSERT(share);

    if (share) {
        m_feedback_icon->setPixmap(KIconLoader::global()->loadIcon(QStringLiteral("dialog-ok"), KIconLoader::Small, 0, KIconLoader::DefaultState));
        m_feedback_icon->setToolTip(i18n("%1 has been mounted successfully.", share->displayString()));

        QList<QTabBar *> list = findChildren<QTabBar *>();
        QDockWidget *shares_dock = findChild<QDockWidget *>(QStringLiteral("SharesViewDockWidget"));

        if (shares_dock) {
            for (int i = 0; i < list.size(); ++i) {
                if (list.at(i)->count() != 0) {
                    for (int j = 0; j < list.at(i)->count(); ++j) {
                        if (QString::compare(shares_dock->windowTitle(), list.at(i)->tabText(j)) == 0 && list.at(i)->currentIndex() != j) {
                            list.at(i)->setTabTextColor(j, palette().highlightedText().color());
                            break;
                        } else {
                            continue;
                        }
                    }
                    continue;
                } else {
                    continue;
                }
            }
        }

        QTimer::singleShot(2000, this, SLOT(slotEndVisualFeedback()));
    }
}

void Smb4KMainWindow::slotVisualUnmountFeedback(const SharePtr &share)
{
    Q_ASSERT(share);

    if (share) {
        m_feedback_icon->setPixmap(KIconLoader::global()->loadIcon(QStringLiteral("dialog-ok"), KIconLoader::Small, 0, KIconLoader::DefaultState));
        m_feedback_icon->setToolTip(i18n("%1 has been unmounted successfully.", share->displayString()));

        QList<QTabBar *> list = findChildren<QTabBar *>();
        QDockWidget *shares_dock = findChild<QDockWidget *>(QStringLiteral("SharesViewDockWidget"));

        if (shares_dock) {
            for (int i = 0; i < list.size(); ++i) {
                if (list.at(i)->count() != 0) {
                    for (int j = 0; j < list.at(i)->count(); ++j) {
                        if (QString::compare(shares_dock->windowTitle(), list.at(i)->tabText(j)) == 0 && list.at(i)->currentIndex() != j) {
                            list.at(i)->setTabTextColor(j, palette().highlightedText().color());
                            break;
                        } else {
                            continue;
                        }
                    }
                    continue;
                } else {
                    continue;
                }
            }
        }

        QTimer::singleShot(2000, this, SLOT(slotEndVisualFeedback()));
    }
}

void Smb4KMainWindow::slotEndVisualFeedback()
{
    QList<QTabBar *> list = findChildren<QTabBar *>();
    QDockWidget *shares_dock = findChild<QDockWidget *>(QStringLiteral("SharesViewDockWidget"));

    if (shares_dock) {
        for (int i = 0; i < list.size(); ++i) {
            if (list.at(i)->count() != 0) {
                for (int j = 0; j < list.at(i)->count(); ++j) {
                    if (QString::compare(shares_dock->windowTitle(), list.at(i)->tabText(j)) == 0) {
                        list.at(i)->setTabTextColor(j, palette().text().color());
                        break;
                    } else {
                        continue;
                    }
                }

                continue;
            } else {
                continue;
            }
        }
    }

    setupMountIndicator();
}

void Smb4KMainWindow::slotSynchronizerAboutToStart(const QString &dest)
{
    statusBar()->showMessage(i18n("Synchronizing %1", dest), 0);

    if (!m_progress_bar->isVisible()) {
        m_progress_bar->setVisible(true);
    }
}

void Smb4KMainWindow::slotSynchronizerFinished(const QString & /*dest*/)
{
    if (!coreIsRunning()) {
        m_progress_bar->setVisible(false);
        m_progress_bar->reset();
        statusBar()->showMessage(i18n("Done."), 2000);
    }
}

void Smb4KMainWindow::slotEnableBookmarkAction()
{
    //
    // Get the focused widget's 'Add Bookmark' action and read its
    // isEnabled() property. Set the action of the main window and the
    // bookmark menu respectively.
    //
    if (m_focusWidget) {
        if (qstrcmp(m_focusWidget->metaObject()->className(), "Smb4KNetworkBrowserDockWidget") == 0) {
            Smb4KNetworkBrowserDockWidget *dockWidget = qobject_cast<Smb4KNetworkBrowserDockWidget *>(m_focusWidget);

            if (dockWidget) {
                QAction *action = dockWidget->actionCollection()->action(QStringLiteral("bookmark_action"));

                if (action) {
                    // Bookmark action of the main window
                    actionCollection()->action(QStringLiteral("bookmark_action"))->setEnabled(action->isEnabled());

                    // Bookmark action of the bookmark menu
                    Smb4KBookmarkMenu *bookmarkMenu = findChild<Smb4KBookmarkMenu *>();

                    if (bookmarkMenu) {
                        bookmarkMenu->setBookmarkActionEnabled(action->isEnabled());
                    }
                }
            }
        } else if (qstrcmp(m_focusWidget->metaObject()->className(), "Smb4KSharesViewDockWidget") == 0) {
            Smb4KSharesViewDockWidget *dockWidget = static_cast<Smb4KSharesViewDockWidget *>(m_focusWidget);

            if (dockWidget) {
                QAction *action = dockWidget->actionCollection()->action(QStringLiteral("bookmark_action"));

                if (action) {
                    // Bookmark action of the main window
                    actionCollection()->action(QStringLiteral("bookmark_action"))->setEnabled(action->isEnabled());

                    // Bookmark action of the bookmark menu
                    Smb4KBookmarkMenu *bookmarkMenu = findChild<Smb4KBookmarkMenu *>();

                    if (bookmarkMenu) {
                        bookmarkMenu->setBookmarkActionEnabled(action->isEnabled());
                    }
                }
            }
        }
    }
}

void Smb4KMainWindow::slotNetworkBrowserVisibilityChanged(bool visible)
{
    QDockWidget *dock = findChild<Smb4KNetworkBrowserDockWidget *>();

    if (dock) {
        if (visible) {
            dock->widget()->setFocus();
        } else {
            dock->widget()->clearFocus();
        }
    }
}

void Smb4KMainWindow::slotSharesViewVisibilityChanged(bool visible)
{
    QDockWidget *dock = findChild<Smb4KSharesViewDockWidget *>();

    if (dock) {
        if (visible) {
            dock->widget()->setFocus();
        } else {
            dock->widget()->clearFocus();
        }
    }
}
