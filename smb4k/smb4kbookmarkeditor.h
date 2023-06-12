/*
 *  smb4kbookmarkeditor  -  Bookmark editor
 *
 *  SPDX-FileCopyrightText: 2023 Alexander Reinholdt <alexander.reinholdt@kdemail.net>
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef SMB4KBOOKMARKEDITOR_H
#define SMB4KBOOKMARKEDITOR_H

// Qt includes
#include <QDialog>
#include <QPushButton>

// application specific includes
#include "smb4kconfigpagebookmarks.h"

class Q_DECL_EXPORT Smb4KBookmarkEditor : public QDialog
{
    Q_OBJECT

public:
    /**
     * Constructor
     */
    explicit Smb4KBookmarkEditor(QWidget *parent, const QList<QVariant> &args);

    /**
     * Destructor
     */
    virtual ~Smb4KBookmarkEditor();

protected Q_SLOTS:
    /**
     * Invoked when the bookmarks changed. Enables the "Save" button
     */
    void slotBookmarksModified();

    /**
     * Invoked when the dialog is accepted and the bookmarks
     * are to be saved.
     */
    void slotAccepted();

    /**
     * Invoked when the dialog is rejected.
     */
    void slotRejected();

private:
    Smb4KConfigPageBookmarks *m_mainWidget;
    QPushButton *m_saveButton;
    QPushButton *m_cancelButton;

};

#endif
