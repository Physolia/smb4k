/*
    smb4kbookmarkmenu  -  Bookmark menu

    SPDX-FileCopyrightText: 2011-2021 Alexander Reinholdt <alexander.reinholdt@kdemail.net>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

// application specific includes
#include "smb4kbookmarkmenu.h"
#include "core/smb4kbookmark.h"
#include "core/smb4kbookmarkhandler.h"
#include "core/smb4kglobal.h"
#include "core/smb4kmounter.h"
#include "core/smb4ksettings.h"
#include "core/smb4kshare.h"

// Qt includes
#include <QDebug>
#include <QLatin1String>
#include <QMenu>

// KDE includes
#include <KI18n/KLocalizedString>
#include <KIconThemes/KIconLoader>

using namespace Smb4KGlobal;

Smb4KBookmarkMenu::Smb4KBookmarkMenu(int type, QObject *parent)
    : KActionMenu(KDE::icon("folder-favorites"), i18n("Bookmarks"), parent)
    , m_type(type)
{
    //
    // Set up the action group for the category menu actions
    //
    m_categories = new QActionGroup(menu());

    //
    // Set up the action group for the bookmarks
    //
    m_bookmarks = new QActionGroup(menu());

    //
    //Set up the mount actions action group
    //
    m_mountActions = new QActionGroup(menu());

    //
    // Add the 'Edit Bookmarks' action
    //
    m_editBookmarks = new QAction(KDE::icon("bookmarks-organize"), i18n("&Edit Bookmarks"), menu());
    m_editBookmarks->setEnabled(!Smb4KBookmarkHandler::self()->bookmarksList().isEmpty());
    connect(m_editBookmarks, SIGNAL(triggered(bool)), SLOT(slotEditActionTriggered(bool)));
    addAction(m_editBookmarks);

    //
    // Add the "Add Bookmark" action, if necessary
    //
    if (m_type == MainWindow) {
        m_addBookmark = new QAction(KDE::icon("bookmark-new"), i18n("Add &Bookmark"), menu());
        m_addBookmark->setEnabled(false);
        connect(m_addBookmark, SIGNAL(triggered(bool)), SLOT(slotAddActionTriggered(bool)));
        addAction(m_addBookmark);
    } else {
        m_addBookmark = nullptr;
    }

    //
    // Add the toplevel "Mount All Bookmarks" action.
    //
    // Show it if there are no (non-empty) categories defined and additionally
    // enable it, if not all bookmarks are already mounted.
    //
    m_toplevelMount = new QAction(KDE::icon("media-mount"), i18n("Mount All Bookmarks"), menu());
    m_toplevelMount->setVisible(false);
    m_toplevelMount->setEnabled(false);
    addAction(m_toplevelMount);
    m_mountActions->addAction(m_toplevelMount);

    QStringList allCategories = Smb4KBookmarkHandler::self()->categoryList();

    if (allCategories.isEmpty() || (allCategories.size() == 1 && allCategories.first().isEmpty())) {
        m_toplevelMount->setVisible(true);

        QList<BookmarkPtr> bookmarks = Smb4KBookmarkHandler::self()->bookmarksList();
        int mountedBookmarks = 0;

        for (const BookmarkPtr &bookmark : qAsConst(bookmarks)) {
            QList<SharePtr> mountedShares = findShareByUrl(bookmark->url());

            if (!mountedShares.isEmpty()) {
                for (const SharePtr &share : qAsConst(mountedShares)) {
                    if (!share->isForeign()) {
                        mountedBookmarks++;
                        break;
                    }
                }
            }
        }

        m_toplevelMount->setEnabled(mountedBookmarks != bookmarks.size());
    }

    //
    // Add a separator
    //
    m_separator = addSeparator();
    m_separator->setVisible(!Smb4KBookmarkHandler::self()->bookmarksList().isEmpty());

    //
    // Add all bookmarks to the menu
    //
    for (const BookmarkPtr &bookmark : Smb4KBookmarkHandler::self()->bookmarksList()) {
        addBookmarkToMenu(bookmark);
    }
    
    // 
    // Adjust mount actions
    // 
    adjustMountActions();

    //
    // Connections
    //
    connect(Smb4KBookmarkHandler::self(), SIGNAL(bookmarkAdded(BookmarkPtr)), SLOT(slotBookmarkAdded(BookmarkPtr)));
    connect(Smb4KBookmarkHandler::self(), SIGNAL(bookmarkRemoved(BookmarkPtr)), SLOT(slotBookmarkRemoved(BookmarkPtr)));
    connect(Smb4KBookmarkHandler::self(), SIGNAL(updated()), SLOT(slotBookmarksUpdated()));
    connect(Smb4KMounter::self(), SIGNAL(mounted(SharePtr)), SLOT(slotEnableBookmark(SharePtr)));
    connect(Smb4KMounter::self(), SIGNAL(unmounted(SharePtr)), SLOT(slotEnableBookmark(SharePtr)));
    connect(m_bookmarks, SIGNAL(triggered(QAction *)), SLOT(slotBookmarkActionTriggered(QAction *)));
    connect(m_mountActions, SIGNAL(triggered(QAction *)), SLOT(slotMountActionTriggered(QAction *)));
}

Smb4KBookmarkMenu::~Smb4KBookmarkMenu()
{
}

void Smb4KBookmarkMenu::refreshMenu()
{
    //
    // Remove all categories from the menu
    //
    while (!m_categories->actions().isEmpty()) {
        QAction *category = m_categories->actions().takeFirst();
        removeAction(category);
        delete category;
    }

    //
    // Remove all remaining toplevel bookmarks
    //
    while (!m_bookmarks->actions().isEmpty()) {
        QAction *bookmark = m_bookmarks->actions().takeFirst();
        removeAction(bookmark);
        delete bookmark;
    }

    //
    // Add all bookmarks to the menu
    //
    for (const BookmarkPtr &bookmark : Smb4KBookmarkHandler::self()->bookmarksList()) {
        addBookmarkToMenu(bookmark);
    }
    
    // 
    // Adjust mount actions
    // 
    adjustMountActions();

    //
    // Enable the "Edit Bookmarks" action
    //
    m_editBookmarks->setEnabled(!Smb4KBookmarkHandler::self()->bookmarksList().isEmpty());

    //
    // Show separator, if necessary
    //
    m_separator->setVisible(!Smb4KBookmarkHandler::self()->bookmarksList().isEmpty());

    //
    // Make sure the correct menu entries are shown
    //
    menu()->update();

    //
    // Work around a display glitch were the first bookmark
    // might not be shown (see also BUG 442187)
    //
    menu()->adjustSize();
}

void Smb4KBookmarkMenu::setBookmarkActionEnabled(bool enable)
{
    m_addBookmark->setEnabled(enable);
}

void Smb4KBookmarkMenu::addBookmarkToMenu(const BookmarkPtr& bookmark)
{
    //
    // The menu where to add the bookmark
    //
    KActionMenu *bookmarkMenu = nullptr;

    //
    // Find the menu where the bookmark is to be added
    //
    if (!bookmark->categoryName().isEmpty()) {
        //
        // Get the list of all categories in the menu
        //
        QList<QAction *> allCategoryActions = m_categories->actions();

        //
        // Now find the appropriate category menu
        //
        for (QAction *category : qAsConst(allCategoryActions)) {
            if (category->text() == bookmark->categoryName()) {
                bookmarkMenu = static_cast<KActionMenu *>(category);
                break;
            }
        }

        //
        // If no category was found in the menu, create one
        //
        if (!bookmarkMenu) {
            //
            // The category submenu
            //
            bookmarkMenu = new KActionMenu(bookmark->categoryName(), menu());
            bookmarkMenu->setIcon(KDE::icon("folder-favorites"));
            addAction(bookmarkMenu);
            m_categories->addAction(bookmarkMenu);

            //
            // The "Mount All Bookmarks" action for this category
            //
            QAction *bookmarkCategoryMount = new QAction(KDE::icon("media-mount"), i18n("Mount All Bookmarks"), bookmarkMenu->menu());
            QMap<QString, QVariant> categoryMountInfo;
            categoryMountInfo["type"] = "category_mount";
            categoryMountInfo["category"] = bookmark->categoryName();
            bookmarkCategoryMount->setData(categoryMountInfo);
            bookmarkMenu->addAction(bookmarkCategoryMount);
            m_mountActions->addAction(bookmarkCategoryMount);

            //
            // A separator
            //
            bookmarkMenu->addSeparator();
        }
    } else {
        bookmarkMenu = this;
    }

    //
    // For sorting purposes, get the display strings of all
    // bookmarks of that are already in the action menu.
    //
    QStringList displayNames;

    for (QAction *entry : m_bookmarks->actions()) {
        if (entry->data().toMap().value("category").toString() == bookmark->categoryName()) {
            displayNames << entry->data().toMap().value("text").toString();
        }
    }

    //
    // Add the display string of the current share as well
    //
    QString displayName;

    if (Smb4KSettings::showCustomBookmarkLabel() && !bookmark->label().isEmpty()) {
        displayName = bookmark->label();
    } else {
        displayName = bookmark->displayString();
    }

    displayNames << displayName;

    //
    // Sort the display strings
    //
    displayNames.sort();

    //
    // Create the bookmark. Disable it if it is mounted.
    //
    QAction *bookmarkAction = new QAction(bookmarkMenu->menu());
    bookmarkAction->setIcon(bookmark->icon());
    bookmarkAction->setText(displayName);

    QMap<QString, QVariant> bookmarkInfo;
    bookmarkInfo["category"] = bookmark->categoryName();
    bookmarkInfo["url"] = bookmark->url();
    bookmarkInfo["text"] = displayName;

    bookmarkAction->setData(bookmarkInfo);
    m_bookmarks->addAction(bookmarkAction);

    QList<SharePtr> mountedShares = findShareByUrl(bookmark->url());

    if (!mountedShares.isEmpty()) {
        for (const SharePtr &share : qAsConst(mountedShares)) {
            if (!share->isForeign()) {
                bookmarkAction->setEnabled(false);
                break;
            }
        }
    }

    //
    // Add the share menu to the action menu at the right place
    //
    if (displayNames.size() == 1) {
        bookmarkMenu->addAction(bookmarkAction);
    } else {
        int index = displayNames.indexOf(displayName, 0);

        if (index != displayNames.size() - 1) {
            QString displayStringBefore = displayNames.at(index + 1);

            for (QAction *action : bookmarkMenu->menu()->actions()) {
                if (action->data().toMap().value("text").toString() == displayStringBefore) {
                    insertAction(action, bookmarkAction);
                    break;
                }
            }
        } else {
            addAction(bookmarkAction);
        }
    }
}

void Smb4KBookmarkMenu::removeBookmarkFromMenu(const BookmarkPtr& bookmark)
{
    //
    // The list of category menus
    //
    QList<QAction *> allCategories = m_categories->actions();

    //
    // Get the action menu
    //
    KActionMenu *bookmarkMenu = nullptr;

    if (!bookmark->categoryName().isEmpty()) {
        for (QAction *action : qAsConst(allCategories)) {
            if (action->data().toMap().value("category").toString() == bookmark->categoryName()) {
                bookmarkMenu = static_cast<KActionMenu *>(action);
                break;
            }
        }
    } else {
        bookmarkMenu = this;
    }

    //
    // Now remove the bookmark
    //
    for (int i = 0; i < m_bookmarks->actions().size(); ++i) {
        QUrl bookmarkUrl = m_bookmarks->actions().at(i)->data().toMap().value("url").toUrl();
        if (bookmark->url().matches(bookmarkUrl, QUrl::RemoveUserInfo|QUrl::RemovePort)) {
            QAction *bookmarkAction = m_bookmarks->actions().takeAt(i);
            bookmarkMenu->removeAction(bookmarkAction);
            delete bookmarkAction;
            break;
        }
    }
}

void Smb4KBookmarkMenu::adjustMountActions()
{
    //
    // Enable and show the mount action(s)
    //
    QStringList allCategories = Smb4KBookmarkHandler::self()->categoryList();

    if (allCategories.isEmpty() || (allCategories.size() == 1 && allCategories.first().isEmpty())) {
        m_toplevelMount->setVisible(true);
        
        QList<BookmarkPtr> allBookmarks = Smb4KBookmarkHandler::self()->bookmarksList();
        int mountedBookmarks = 0;

        for (const BookmarkPtr &bookmark : qAsConst(allBookmarks)) {
            QList<SharePtr> mountedShares = findShareByUrl(bookmark->url());

            if (!mountedShares.isEmpty()) {
                for (const SharePtr &share : qAsConst(mountedShares)) {
                    if (!share->isForeign()) {
                        mountedBookmarks++;
                        break;
                    }
                }
            }
        }

        m_toplevelMount->setEnabled(mountedBookmarks != allBookmarks.size());
    } else {
        m_toplevelMount->setVisible(false);
        m_toplevelMount->setEnabled(false);
        
        QList<QAction *> allMountActions = m_mountActions->actions();
        int mountedBookmarks = 0;

        for (const QString &category : qAsConst(allCategories)) {
            QList<BookmarkPtr> bookmarks = Smb4KBookmarkHandler::self()->bookmarksList(category);
            
            for (const BookmarkPtr &bookmark : bookmarks) {
                QList<SharePtr> mountedShares = findShareByUrl(bookmark->url());
                
                if (!mountedShares.isEmpty()) {
                    for (const SharePtr &share : qAsConst(mountedShares)) {
                        if (!share->isForeign()) {
                            mountedBookmarks++;
                            break;
                        }
                    }
                }
            }
            
            for (QAction *action : allMountActions) {
                if (action->data().toMap().value("categrory").toString() == category && action->data().toMap().value("type") == "category_mount") {
                    action->setEnabled(bookmarks.size() != mountedBookmarks);
                    break;
                }
            }
            
            mountedBookmarks = 0;
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
// SLOT IMPLEMENTATIONS
/////////////////////////////////////////////////////////////////////////////

void Smb4KBookmarkMenu::slotEditActionTriggered(bool /*checked*/)
{
    Smb4KBookmarkHandler::self()->editBookmarks();
}

void Smb4KBookmarkMenu::slotAddActionTriggered(bool /*checked*/)
{
    emit addBookmark();
}

void Smb4KBookmarkMenu::slotMountActionTriggered(QAction* action)
{
    //
    // The list of bookmarks
    //
    QList<BookmarkPtr> bookmarks;

    //
    // Get the bookmarks
    //
    if (action == m_toplevelMount) {
        bookmarks = Smb4KBookmarkHandler::self()->bookmarksList("");
    } else {
        bookmarks = Smb4KBookmarkHandler::self()->bookmarksList(action->data().toMap().value("category").toString());
    }

    //
    // Prepare the list of shares
    //
    QList<SharePtr> mounts;

    for (const BookmarkPtr &bookmark : qAsConst(bookmarks)) {
        SharePtr share = SharePtr(new Smb4KShare());
        share->setHostName(bookmark->hostName());
        share->setShareName(bookmark->shareName());
        share->setWorkgroupName(bookmark->workgroupName());
        share->setHostIpAddress(bookmark->hostIpAddress());
        share->setLogin(bookmark->login());
        mounts << share;
    }

    //
    // Mount the bookmarks
    //
    Smb4KMounter::self()->mountShares(mounts);

    //
    // Clear the list of shares
    //
    while (!mounts.isEmpty()) {
        mounts.takeFirst().clear();
    }
}

void Smb4KBookmarkMenu::slotBookmarkActionTriggered(QAction *action)
{
    QMap<QString, QVariant> info = action->data().toMap();
    QString bookmarkCategory = info.value("category").toString();
    QUrl url = info.value("url").toUrl();

    BookmarkPtr bookmark = Smb4KBookmarkHandler::self()->findBookmarkByUrl(url);

    if (bookmark && bookmarkCategory == bookmark->categoryName()) {
        SharePtr share = SharePtr(new Smb4KShare());
        share->setHostName(bookmark->hostName());
        share->setShareName(bookmark->shareName());
        share->setWorkgroupName(bookmark->workgroupName());
        share->setHostIpAddress(bookmark->hostIpAddress());
        share->setLogin(bookmark->login());
        Smb4KMounter::self()->mountShare(share);
        share.clear();
    }
}

void Smb4KBookmarkMenu::slotBookmarkAdded(const BookmarkPtr& bookmark)
{
    addBookmarkToMenu(bookmark);
}

void Smb4KBookmarkMenu::slotBookmarkRemoved(const BookmarkPtr& bookmark)
{
    removeBookmarkFromMenu(bookmark);
}

void Smb4KBookmarkMenu::slotBookmarksUpdated()
{
    //
    // Enable the "Edit Bookmarks" action if necessary
    //
    QList<BookmarkPtr> allBookmarks = Smb4KBookmarkHandler::self()->bookmarksList();
    m_editBookmarks->setEnabled(!allBookmarks.isEmpty());

    //
    // Enable and show the separator if necessary
    //
    m_separator->setEnabled(!m_bookmarks->actions().isEmpty());
    m_separator->setVisible(!m_bookmarks->actions().isEmpty());
    
    // 
    // Adjust mount actions
    // 
    adjustMountActions();

    //
    // Make sure the correct menu entries are shown
    //
    menu()->update();

    //
    // Work around a display glitch where the first mounted
    // share might not be shown (BUG 442187)
    //
    menu()->adjustSize();
}

void Smb4KBookmarkMenu::slotEnableBookmark(const SharePtr &share)
{
    if (!share->isForeign() && !m_bookmarks->actions().isEmpty()) {
        //
        // Enable or disable the bookmark
        //
        QList<QAction *> actions = m_bookmarks->actions();
        QString bookmarkCategory;

        for (QAction *a : qAsConst(actions)) {
            QUrl bookmarkUrl = a->data().toMap().value("url").toUrl();

            if (share->url().matches(bookmarkUrl, QUrl::RemoveUserInfo | QUrl::RemovePort)) {
                a->setEnabled(!share->isMounted());
                bookmarkCategory = a->data().toMap().value("category").toString();
                break;
            }
        }
        
        //
        // Adjust mount actions
        // 
        adjustMountActions();
    }
}
